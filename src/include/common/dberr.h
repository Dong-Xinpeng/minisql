#ifndef MINISQL_DBERR_H
#define MINISQL_DBERR_H

enum dberr_t {
  DB_SUCCESS = 0,
  DB_FAILED,
  DB_TABLE_ALREADY_EXIST,
  DB_TABLE_NOT_EXIST,
  DB_INDEX_ALREADY_EXIST,
  DB_INDEX_NOT_FOUND,
  DB_COLUMN_NAME_NOT_EXIST,
  DB_KEY_NOT_FOUND,
  DB_COLUMN_NOT_UNIQUE,
  DB_TUPLE_TOO_LARGE,
  DB_PK_DUPLICATE,
  DB_UNI_KEY_DUPLICATE,
};

#endif //MINISQL_DBERR_H