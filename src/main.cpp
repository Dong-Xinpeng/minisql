#include <cstdio>
#include "executor/execute_engine.h"
#include "glog/logging.h"
#include "parser/syntax_tree_printer.h"
#include "utils/tree_file_mgr.h"
#include <fstream> // for debug

extern "C" {
int yyparse(void);
FILE *yyin;
#include "parser/minisql_lex.h"
#include "parser/parser.h"
}

// #define ENABLE_PARSER_DEBUG // debug
// #define ENABLE_FILE_INPUT // debug, read cmds from file, only support one line 
// #define ENABLE_FILE_OUTPUT // debug, output to "output.txt"

void InitGoogleLog(char *argv) {
  FLAGS_logtostderr = true;
  FLAGS_colorlogtostderr = true;
  google::InitGoogleLogging(argv);
}

void InputCommand(char *input, const int len) {
  memset(input, 0, len);
  printf("minisql > ");
  int i = 0;
  char ch;
  while ((ch = getchar()) != ';') {
    input[i++] = ch;
  }
  input[i] = ch;    // ;
  getchar();        // remove enter
}

int main(int argc, char **argv) {
  InitGoogleLog(argv[0]);
  // command buffer
  const int buf_size = 1024;
  char cmd[buf_size];
  // execute engine
  ExecuteEngine engine;
  // for print syntax tree
  // string prefix = "stree_"; // for test
  string prefix = "syntax_tree_";
  TreeFileManagers syntax_tree_file_mgr(prefix);
  [[maybe_unused]] uint32_t syntax_tree_id = 0;
  #ifdef ENABLE_FILE_INPUT
    std::fstream cmdIn("./test.sql", std::ios::in); // get command from file
  #endif

  #ifdef ENABLE_FILE_OUTPUT
  	ofstream fout("output.txt");
	  cout.rdbuf(fout.rdbuf());
  #endif

  while (1) {
    #ifdef ENABLE_FILE_INPUT
      // read from file
      memset(cmd, 0, buf_size);
      cmdIn.getline(cmd, buf_size);
      if (cmd[0] == '\0') {
        continue;
      }
      if (cmd[0] == '-') {
        cout << "\n[COMMENT] " << cmd << endl;
        continue;
      }
      cout << "\n[CMD] " << cmd << endl;
    #else
      // read from buffer
      InputCommand(cmd, buf_size);

      // //see the Syntax tree
      // SyntaxTreePrinter printer(MinisqlGetParserRootNode()); // error if two printer
      // printer.PrintTree(syntax_tree_file_mgr[syntax_tree_id++]);
      
    #endif
    // create buffer for sql input
    YY_BUFFER_STATE bp = yy_scan_string(cmd);
    if (bp == nullptr) {
      LOG(ERROR) << "Failed to create yy buffer state." << std::endl;
      exit(1);
    }
    yy_switch_to_buffer(bp);

    // init parser module
    MinisqlParserInit();

    // parse
    yyparse();

    // parse result handle
    if (MinisqlParserGetError()) {
      // error
      printf("%s\n", MinisqlParserGetErrorMessage());
    } else {
      #ifdef ENABLE_PARSER_DEBUG
        printf("[INFO] Sql syntax parse ok!\n");
        SyntaxTreePrinter printer(MinisqlGetParserRootNode());
        printer.PrintTree(syntax_tree_file_mgr[syntax_tree_id++]);
        fstream fout(prefix+std::to_string(syntax_tree_id-1)+".txt",
                     std::ios::app);
        fout << endl << "The cmd is:" << endl << cmd << endl;
        fout.close();
      #endif
    }

    ExecuteContext context;
    engine.Execute(MinisqlGetParserRootNode(), &context);
    // sleep(1); // probably not needed

    // clean memory after parse
    MinisqlParserFinish();
    yy_delete_buffer(bp);
    yylex_destroy();

    // quit condition
    if (context.flag_quit_) {
      printf("bye!\n");
      break;
    }
  }
  #ifdef ENABLE_FILE_INPUT
    cmdIn.close();
  #endif
  #ifdef ENABLE_FILE_OUTPUT
  	fout.close();
  #endif
  return 0;
}