/*
  * testlibpq.c
  *
  * Test the C version of libpq, the PostgreSQL frontend
  * library.
  */
#include <cstdio>
#include <libpq-fe.h>

#include <sstream>

using namespace std;

void
exit_nicely(PGconn *conn)
{
  PQfinish(conn);
  exit(1);
}

int main(int argc, char **argv) {
  char       *pghost,
    *pgport,
    *pgoptions,
    *pgtty;
  char       *dbName;
  int         nFields;
  int         i,j;

  /* FILE *debug; */

  PGconn     *conn;
  PGresult   *res;

  dbName = "host='last' dbname='gergram' user='kiefer' password='kiefer'";
  /* make a connection to the database */
  conn = PQconnectdb(dbName);

  /*
   * check to see that the backend connection was successfully made
   */
  if (PQstatus(conn) == CONNECTION_BAD)
    {
      fprintf(stderr, "Connection to database '%s' failed.\n", dbName);
      fprintf(stderr, "%s", PQerrorMessage(conn));
      exit_nicely(conn);
    }

  /* debug = fopen("/tmp/trace.out","w"); */
  /* PQtrace(conn, debug);  */

  /* start a transaction block */
  res = PQexec(conn, "BEGIN");
  if (!res || PQresultStatus(res) != PGRES_COMMAND_OK)
    {
      fprintf(stderr, "BEGIN command failed\n");
      PQclear(res);
      exit_nicely(conn);
    }

  /*
   * should PQclear PGresult whenever it is no longer needed to avoid
   * memory leaks
   */
  PQclear(res);

  /*
   * fetch rows from the pg_database, the system catalog of
   * databases
   */
  char *mode = "gergram";
  char *realmode = (char *) malloc(2*(strlen(mode) + 1));
  PQescapeString (realmode, mode, strlen(mode));

  string command =
    (string) "SELECT slot,field,path,type FROM dfn WHERE mode='"
    + realmode + "' OR mode IS NULL";

  res = PQexec(conn, command.c_str());
  if (!res || PQresultStatus(res) != PGRES_TUPLES_OK)
    {
      fprintf(stderr, "SELECT dfn fields command failed\n");
      PQclear(res);
      exit_nicely(conn);
    }
/*   PQclear(res); */
/*   res = PQexec(conn, "FETCH ALL in mycursor"); */
/*   if (!res || PQresultStatus(res) != PGRES_TUPLES_OK) */
/*     { */
/*       fprintf(stderr, "FETCH ALL command didn't return tuples properly\n"); */
/*       PQclear(res); */
/*       exit_nicely(conn); */
/*     } */

  /* first, print out the attribute names */
  nFields = PQnfields(res);
  for (i = 0; i < nFields; ++i)
    printf("%-15s", PQfname(res, i));
  printf("\n\n");

  /* next, print out the rows */
  for (i = 0; i < PQntuples(res); ++i)
    {
      for (j = 0; j < nFields; ++j)
        printf("%-15s", PQgetvalue(res, i, j));
      printf("\n");
    }
  /* next, print out the rows */
  for (int row = 0; row < PQntuples(res); ++row)
    {
      commandstring = "type, orth"
      if (PQgetvalue(res, row, SLOT_COLUMN) != "unifs"
          || PQgetvalue(res, row, NAME_COLUMN) == "type")
        continue;
      for (int col = 0; col < nFields; ++col) {
        char *slotname = PQgetvalue(res, row, NAME_COLUMN);
        int slotnamelen = strlen(slotname);
        char *sqlslotname = new char[2*(slotnamelen + 1)];
        PQescapeString(sqlslotname, slotname, slotnamelen);
        commandstring += ", " + sqlslotname;
        char *pathstring = PQgetvalue(res, row, PATH_COLUMN);
        string path = lisp2petpath(pathstring);

        col_type_t col_type;
        char *typestring = PQgetvalue(res, row, TYPE_COLUMN);
        // typestring can be mixed, symbol, string or string-fs
        if (typestring[0] == 'm')
          col_type = COL_MIXED;
        else
          if (typestring[1] == 'y')
            col_type = COL_SYMBOL;
          else
            if (typestring[6] == '-')
              col_type = COL_STRING_FS;
            else
              col_type = COL_STRING;
        printf("%-15s", PQgetvalue(res, i, j));
      printf("\n");
    }
  PQclear(res);

  /* close the cursor */
  //res = PQexec(conn, "CLOSE mycursor");
  //PQclear(res);

  /* commit the transaction */
  res = PQexec(conn, "COMMIT");
  PQclear(res);

  /* close the connection to the database and cleanup */
  PQfinish(conn);

  /* fclose(debug); */
  return 0;
}


