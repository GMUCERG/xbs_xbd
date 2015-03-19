#! /bin/sh
rm data.db
sqlite3 data.db < schema.sql 
