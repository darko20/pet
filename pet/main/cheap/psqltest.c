/*
  * testlibpq.c
  *
  * Test the C version of libpq, the PostgreSQL frontend
  * library.
  */
 #include <stdio.h>
 #include <libpq-fe.h>
 
 void
 exit_nicely(PGconn *conn)
 {
     PQfinish(conn);
     exit(1);
 }
 
 main()
 {
     char       *pghost,
                *pgport,
                *pgoptions,
                *pgtty;
     char       *dbName;
     int         nFields;
     int         i,
                 j;
 
     /* FILE *debug; */
 
     PGconn     *conn;
     PGresult   *res;
 
     dbName = "host='127.0.0.1' dbname='lingo' user='guest' password='guest'";
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
     res = PQexec(conn, "DECLARE mycursor CURSOR FOR SELECT * FROM erg");
     if (!res || PQresultStatus(res) != PGRES_COMMAND_OK)
     {
         fprintf(stderr, "DECLARE CURSOR command failed\n");
         PQclear(res);
         exit_nicely(conn);
     }
     PQclear(res);
     res = PQexec(conn, "FETCH ALL in mycursor");
     if (!res || PQresultStatus(res) != PGRES_TUPLES_OK)
     {
         fprintf(stderr, "FETCH ALL command didn't return tuples properly\n");
         PQclear(res);
         exit_nicely(conn);
     }
 
     /* first, print out the attribute names */
     nFields = PQnfields(res);
     for (i = 0; i < nFields; i++)
         printf("%-15s", PQfname(res, i));
     printf("\n\n");
 
     /* next, print out the rows */
     for (i = 0; i < PQntuples(res); i++)
     {
         for (j = 0; j < nFields; j++)
             printf("%-15s", PQgetvalue(res, i, j));
         printf("\n");
     }
     PQclear(res);
 
     /* close the cursor */
     res = PQexec(conn, "CLOSE mycursor");
     PQclear(res);
 
     /* commit the transaction */
     res = PQexec(conn, "COMMIT");
     PQclear(res);
 
     /* close the connection to the database and cleanup */
     PQfinish(conn);
 
     /* fclose(debug); */
     return 0;
 
 }
