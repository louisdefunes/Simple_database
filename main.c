#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#define DEFAULT_DATA 11
#define DEFAULT_ROWS 100

int max_data = DEFAULT_DATA;
int max_rows = DEFAULT_ROWS;

struct Address {
	int id; //4
	int set; // 4
	char *name; //11
	char *email; //11 
};// 4 + 4 + 11 + 11 = 30 . Nie dzieli sie przez 4, wiec dodane przez padding bedzie 2, czyli struktura bedzie miala rozmiar 32

struct Database {
	int max_rows;
	int max_data;
	struct Address *rows; 
};

struct Connection {
	FILE *file;
	struct Database *db;
};

void Database_close(struct Connection *conn);

void die(char *message, struct Connection *conn)
{
	if (errno) {
		perror(message);
	} else {
		printf("ERROR: %s\n", message);
	}
	
	if (conn)
		Database_close(conn);	
	exit(1);
}

void Address_print(struct Address *addr) 
{
	printf("%d %s %s\n", addr->id, addr->name, addr->email);
}

void Database_load(struct Connection *conn)
{
	struct Database *db = conn->db;
	int rc = fread(&db->max_data, sizeof(int), 1, conn->file);
	if (rc != 1)
		die("Failed to load database", conn);

	rc = fread(&db->max_rows, sizeof(int), 1, conn->file);
	if (rc != 1)
		die("Failed to load database", conn);
	
	//zobacz Database_write
	conn->db->rows = malloc(sizeof(struct Address) * conn->db->max_rows);
	//rc = fread(conn->db->rows, sizeof(struct Address), conn->db->max_rows, conn->file);
	//if (rc != conn->db->max_rows)
		//die("Failed to load database", conn);
	
	for (int i = 0; i < db->max_rows; i++) {
		struct Address *addr = &db->rows[i];

		if(fread(&addr->id, sizeof(int), 1, conn->file) != 1) die("Read id failed", conn);
		if(fread(&addr->set, sizeof(int), 1, conn->file) != 1) die("Read set failed", conn);

		addr->name = malloc(db->max_data);
		addr->email = malloc(db->max_data);

		if (!addr->name || !addr->email) die("Memory fail", conn);

		if(fread(addr->name, sizeof(char), db->max_data, conn->file) != db->max_data) die("Read name failed", conn);
		if(fread(addr->email, sizeof(char), db->max_data, conn->file) != db->max_data) die("Read email failed", conn);
		
	}
}
	
struct Connection *Database_open(const char *filename, char mode)
{
	struct Connection *conn = malloc(sizeof(struct Connection));
	if (!conn)
		die("Memory error", conn);

	conn->db = malloc(sizeof(struct Database));
	
	if (!conn->db)
		die("Memory error", conn);
	
	if (mode == 'c') {
		conn->db->max_data = max_data;
		conn->db->max_rows = max_rows;
		conn->file = fopen(filename, "w");
	} else {
		conn->file = fopen(filename, "r+");

		if (conn->file) {
			Database_load(conn);
		}
	}

	if (!conn->file)
		die("Failed to open the file", conn);

	return conn;
}

void Database_close(struct Connection *conn)
{
	if (conn) {
		if (conn->file)
			fclose(conn->file);

		if (conn->db) {
			if (conn->db->rows) {
				for (int i = 0; i < conn->db->max_rows; i++) {
					struct Address *addr = &conn->db->rows[i];

					if (addr->name) free(addr->name);
					if (addr->email) free(addr->email);
				}
				free(conn->db->rows);
			}
			free(conn->db);
		}
		free(conn);
	}
}

void Database_write(struct Connection *conn)
{
	struct Database *db = conn->db;

	//przejdz na poczatek pliku
	rewind(conn->file);

	//zapisz max_data i max_rows
	if(fwrite(&db->max_data, sizeof(int), 1, conn->file) != 1) die("Write max_data failed", conn);
	if(fwrite(&db->max_rows, sizeof(int), 1, conn->file) != 1) die("Write max_rows failed", conn);

	//to nie zadziala bo zapiszesz tutaj adresy wskaznikow name i email struktury Address, a nie wskazywane przez nie wartosci, trzeba stworzyc petle zapisujaca kazdy element po kolei :I :((. Po zostawieniu tego tak, wyprintowanie np ./main dbfile g 0 da nam po prostu numer 0 bo nie zapiszemy poprawnie nazwy name i email
	//rc = fwrite(conn->db->rows, sizeof(struct Address), conn->db->max_rows, conn->file);

	for (int i = 0; i < db->max_rows; i++) {
		//tutaj jest ten addr tworzony dla wygody. Definiujemy wskaznik do struktury zeby uzywac strzalek zamiast kropek bo wygodniej. A że db->rows[i] to jest dereferencja, to musimy dać '&' aby pasowało do wskaznika *addr
		struct Address *addr = &db->rows[i];

		if(fwrite(&addr->id, sizeof(int), 1, conn->file) != 1) die("Write id failed", conn);

		if(fwrite(&addr->set, sizeof(int), 1, conn->file) != 1) die("Write set failed", conn);

		//bufor do przechowywania danych, trzeba pamietac o zerowaniu go
		char *buffer = calloc(db->max_data, sizeof(char));

		//jezeli set i istnieje name, przekopiuj do bufora
		if (addr->set && addr->name) {
			strncpy(buffer, addr->name, max_data);	
		}

		if(fwrite(buffer, db->max_data, 1, conn->file) != 1) die("Write name fail", conn);

		//po przeslaniu zeruj bufor
		memset(buffer, 0, db->max_data);

		//tak jak wyzej
		if (addr->set && addr->email) {
			strncpy(buffer, addr->email, max_data);
		}

		if(fwrite(buffer, db->max_data, 1, conn->file) != 1) die("Write email fail", conn);

		free(buffer);
	}
	if(fflush(conn->file) != 0) die("Fflush failed", conn);
}

void Database_create(struct Connection *conn, int max_rows)
{
	conn->db->rows = malloc(sizeof(struct Address) * max_rows);
	if (!conn->db->rows) die("Failed to allocate memory for rows", conn);

	int i = 0;

	for (i = 0; i < conn->db->max_rows; i++) {
		//make a prototype to initialize it
		struct Address addr = {.id = i,.set = 0 };
		// then just assign it
		conn->db->rows[i] = addr;
	}
}

void Database_set(struct Connection *conn, int id, const char *name, const char *email)
{
	struct Address *addr = &conn->db->rows[id]; //to znaczy &(conn->db->rows[id]) a nie (&conn)->db->rows[id]
	if (addr->set)
		die("Already set, delete it first", conn);

	addr->set = 1;
	
	addr->name = strdup(name);
	addr->email = strdup(email);
	printf("name: %s\nemail: %s\n", addr->name, addr->email);
}

void Database_get(struct Connection *conn, int id)
{
	struct Address *addr = &conn->db->rows[id];
	if (addr->set) {
		Address_print(addr);
	} else {
		die("ID is not set", conn);
	}
}

void Database_delete(struct Connection *conn, int id)
{
	struct Address addr = {.id = id,.set = 0};
	conn->db->rows[id] = addr;
}

void Database_list(struct Connection *conn)
{
	int i = 0;
	struct Database *db = conn->db;

	for(i = 0; i < db->max_rows; i++) {
		struct Address *cur = &db->rows[i];

		if(cur->set) {
			Address_print(cur);
		}
	}
}

int main(int argc, char *argv[])
{
	if (argc < 3) {
		printf("type ./main <dbfile> help to see available options\n");
		die("USAGE: ./main <dbfile> <action> [action params]", 0);
	}

	char *filename = argv[1];
	char action = argv[2][0];

	struct Connection *conn = Database_open(filename, action);
	int id = 0;

	if (argc > 3 && action != 'c') id = atoi(argv[3]);
	if (id >= conn->db->max_rows) die("There's not that many records", conn);

	switch(action) {
		case 'c':
			if (argc != 5) {
				printf("using default values:\n");
				printf("\tDEFAULT_ROWS: %d\n", DEFAULT_ROWS);
				printf("\tDEFAULT_DATA: %d\n", DEFAULT_DATA);
				printf("If you want to specify those variables, use:\n ./main <dbfile> c <max_rows> <max_data>\n");
			} else {
				conn->db->max_rows = atoi(argv[3]);
				conn->db->max_data = atoi(argv[4]);
			}
			Database_create(conn, conn->db->max_rows);
			Database_write(conn);
			break;

		case 'g':
			if (argc != 4)
				die("Need an id to get", conn);

			Database_get(conn, id);
			break;

		case 's':
			if (argc != 6)
				die("Need id, name, email to set", conn);

			Database_set(conn, id, argv[4], argv[5]);
			Database_write(conn);
			break;

		case 'd':
			if (argc != 4)
				die("Need id to delete", conn);

			Database_delete(conn, id);
			Database_write(conn);
			break;

		case 'l':
			Database_list(conn);
			break;

		default:
			printf("Available options:\n");
			printf("\t'c' <max_rows> <max_data_size>: Create database with variable record number and data size available\n");
		        printf("\t'g' <id>: Read record in database by providing an id\n");
			printf("\t's' <id> <name> <email>: Set record in database by providing id, name and email\n");
			printf("\t'd' <id>: Delete record with id\n");
			printf("\t'l': List database\n");	
			die("quitting....", conn);
	}

	Database_close(conn);

	return 0;
}
