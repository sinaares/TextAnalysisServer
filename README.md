# TextAnalysisServer

This is a Text Analysis Server implemented in C. It processes input text from clients, checks for spelling errors, and suggests corrections based on a predefined dictionary. The server uses the Levenshtein distance algorithm to find the closest matches for misspelled words.

Features:

Input Validation: Ensures input strings adhere to specified limits and contain only supported characters.


Spell Checking: Compares words against a dictionary and suggests corrections for misspelled words.


Dynamic Dictionary: Allows clients to add new words to the dictionary.


Threaded Processing: Handles each word in the input string independently for efficient processing.



User-Friendly Interaction: Provides clear feedback and options to continue or terminate the session.


How It Works:

The server reads a dictionary file (basic_english_2000.txt) into memory.


It listens for client connections on a specified port (60000).


Clients send input strings to the server.


The server processes each word, checks it against the dictionary, and suggests corrections if necessary.


Clients can choose to add new words to the dictionary or accept suggested corrections.


The server sends the corrected sentence back to the client.


Usage:

Compile the server code:


gcc -o text_analysis_server text_analysis_server.c -lpthread

Run the server:


./text_analysis_server


Connect to the server using a client (e.g., telnet or a custom client):


telnet localhost 60000

Enter an input string and follow the server's prompts.

Requirements:

A dictionary file named basic_english_2000.txt containing a list of valid words.


A C compiler (e.g., gcc).


POSIX-compliant system (for threading and socket APIs).

