#include "storage/table_heap.h"


bool TableHeap::InsertTuple(Row &row, Transaction *txn) {
  page_id_t i = first_page_id_;
  auto page = reinterpret_cast<TablePage *>(buffer_pool_manager_->FetchPage(i));
  int lastI = i;
  // first fit
  while(i!=INVALID_PAGE_ID){
    page=reinterpret_cast<TablePage *>(buffer_pool_manager_->FetchPage(i));
    if(page->InsertTuple(row,schema_,txn,lock_manager_,log_manager_)){
      buffer_pool_manager_->UnpinPage(page->GetTablePageId(),true);
      return true;
    }
    lastI = i;
    i = page->GetNextPageId();
    buffer_pool_manager_->UnpinPage(page->GetTablePageId(),false);
  }
  // need to allocate a new page, i=invalid page id
  page=reinterpret_cast<TablePage *>(buffer_pool_manager_->NewPage(i));
  page->Init(i,lastI,log_manager_,txn);
  // set next page id
  auto lastPage=reinterpret_cast<TablePage *>(buffer_pool_manager_->FetchPage(lastI));
  lastPage->SetNextPageId(i);
  buffer_pool_manager_->UnpinPage(lastI, true);
  // insert to new page
  if(page->InsertTuple(row,schema_,txn,lock_manager_,log_manager_)){
    buffer_pool_manager_->UnpinPage(i, true);
    return true;
  }
  buffer_pool_manager_->UnpinPage(i, true);
  return false;
}

bool TableHeap::MarkDelete(const RowId &rid, Transaction *txn) {
  // Find the page which contains the tuple.
  auto page = reinterpret_cast<TablePage *>(buffer_pool_manager_->FetchPage(rid.GetPageId()));
  // If the page could not be found, then abort the transaction.
  if (page == nullptr) {
    return false;
  }
  // Otherwise, mark the tuple as deleted.
  page->WLatch();
  page->MarkDelete(rid, txn, lock_manager_, log_manager_);
  page->WUnlatch();
  buffer_pool_manager_->UnpinPage(page->GetTablePageId(), true);
  return true;
}

bool TableHeap::UpdateTuple(Row &row, const RowId &rid, Transaction *txn) {
  auto page=reinterpret_cast<TablePage *>(buffer_pool_manager_ ->FetchPage(rid.GetPageId()));//get the page
  if(page==nullptr){
    return false;
  }
  Row *oldRow=new Row(rid);//get the old row
  int type=page->UpdateTuple(row,oldRow,schema_,txn,lock_manager_,log_manager_);//get the update result
  switch(type){
    case 1:buffer_pool_manager_->UnpinPage(page->GetTablePageId(), true);return false;
    case 2:buffer_pool_manager_->UnpinPage(page->GetTablePageId(), true);return false;//invalid update
    case 0:buffer_pool_manager_->UnpinPage(page->GetTablePageId(), true);return true;
    //not enough space for update
    case 3:if(page->MarkDelete(rid,txn,lock_manager_,log_manager_)){
      TablePage *oldPage=page;
      int i=first_page_id_;
      int preI=i;
      while(i!=INVALID_PAGE_ID){
        page=reinterpret_cast<TablePage *>(buffer_pool_manager_ ->FetchPage(i));
        if(page->InsertTuple(row,schema_,txn,lock_manager_,log_manager_)){
          oldPage->ApplyDelete(rid,txn,log_manager_);
          buffer_pool_manager_->UnpinPage(page->GetTablePageId(), true);
          return true;
        }else{
          preI=i;
          i=page->GetNextPageId();
          buffer_pool_manager_->UnpinPage(page->GetTablePageId(), false);
        }
      }
      //create a new page
      page=reinterpret_cast<TablePage *>(buffer_pool_manager_->NewPage(i));
      page->Init(preI,i,log_manager_,txn);
      if(page->InsertTuple(row,schema_,txn,lock_manager_,log_manager_)){
          oldPage->ApplyDelete(rid,txn,log_manager_);
          buffer_pool_manager_->UnpinPage(page->GetTablePageId(), true);
          return true;
      }
      buffer_pool_manager_->UnpinPage(page->GetTablePageId(), true);
      return false;
    }else{
      buffer_pool_manager_->UnpinPage(page->GetTablePageId(), false);
      return false;
    }
  }
  buffer_pool_manager_->UnpinPage(page->GetTablePageId(), false);
  return false;
}

void TableHeap::ApplyDelete(const RowId &rid, Transaction *txn) {
  // Step1: Find the page which contains the tuple.
  // Step2: Delete the tuple from the page.
  auto page=reinterpret_cast<TablePage *>(buffer_pool_manager_->FetchPage(rid.GetPageId()));
  page->ApplyDelete(rid,txn,log_manager_);
  buffer_pool_manager_->UnpinPage(page->GetTablePageId(), true);
}

void TableHeap::RollbackDelete(const RowId &rid, Transaction *txn) {
  // Find the page which contains the tuple.
  auto page = reinterpret_cast<TablePage *>(buffer_pool_manager_->FetchPage(rid.GetPageId()));
  assert(page != nullptr);
  // Rollback the delete.
  page->WLatch();
  page->RollbackDelete(rid, txn, log_manager_);
  page->WUnlatch();
  buffer_pool_manager_->UnpinPage(page->GetTablePageId(), true);
}

void TableHeap::FreeHeap() {
  int i=first_page_id_;
  while(i!=INVALID_PAGE_ID){
    auto page=reinterpret_cast<TablePage *>(buffer_pool_manager_->FetchPage(i));
    i=page->GetNextPageId();
    // delete page
    buffer_pool_manager_->UnpinPage(page->GetTablePageId(), false);
    buffer_pool_manager_->DeletePage(page->GetTablePageId());
  }
}

bool TableHeap::GetTuple(Row *row, Transaction *txn) {
  auto page=reinterpret_cast<TablePage *>(buffer_pool_manager_->FetchPage(row->GetRowId().GetPageId()));
  if(page->GetTuple(row,schema_,txn,lock_manager_)){
    buffer_pool_manager_->UnpinPage(page->GetTablePageId(), true);
    return true;
  }else{
    buffer_pool_manager_->UnpinPage(page->GetTablePageId(), false);
    return false;
  }
}

TableIterator TableHeap::Begin(Transaction *txn) {
  return TableIterator(this);
}

TableIterator TableHeap::End() {
  return TableIterator(this,nullptr);
}
