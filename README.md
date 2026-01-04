C Database (Dynamic Memory Implementation)
This project is an improved implementation of the database exercise (Ex17) from "Learn C The Hard Way" by Zed A. Shaw.

While the original exercise relies on fixed-size structures and static arrays, this version has been significantly refactored to demonstrate dynamic memory management and pointer manipulation.

Key Technical Differences
Dynamic Allocation: Replaced static arrays (e.g., char name[512]) with pointers (char *). Memory for records and string fields is allocated dynamically (malloc/calloc) based on runtime parameters.

Deep Serialization: Implemented a custom serialization protocol. Unlike the original approach—which simply dumps the struct memory to disk—this version manually handles the writing and reading of pointer-referenced data to ensure the actual strings are persisted, not just their memory addresses.

Memory Management: Features a comprehensive cleanup routine that recursively frees memory for both the row structures and their nested string fields to ensure no memory leaks occur.
