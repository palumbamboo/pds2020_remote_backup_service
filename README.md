# REMOTE BACKUP SERVICE

Service is composed by two components: Client and Server. Each one works independently. For each one you can visualize the helper running the command --help (-h)


## Protocol definition
Definition of a simple protocol for the exchange of messages and data over a TCP channel. Each message starts with the command to execute, followed by the necessary parameters divided by a space and each message ends with a double "\n"

# Commands

## CREATE

Used by the client to request the creation of a new file in its own remote filesystem folder. Creating a file is a destructive operation if the file already exists.

**USAGE**:     `CREATE | | clientID | | file path | | file size | | file data`

- **CREATE** `int` -> value **0**
- **clientID** `std::string` -> previously saved random generated value long 64 characters
- **file path** `std::string` converted in `std::filesystem::path` -> path to interested file (server will check path validity against its file system)
- **file size** `size_t` -> file size in client local filesystem
- **file data** `boost::asio::streambuf` -> request buffer with data inside

**RESPONSE**: **1** file correctly created, **0** no changes to the server filesystem.		

## REMOVE
Used by the client to request the removal of a file from the server's filesystem. If the file is not present in memory, no exception is raised, the same if the request is made multiple times.

**USAGE**:     `REMOVE | | clientID | | file path`

- **REMOVE** `int` -> value **1**
- **clientID** `std::string` -> previously saved random generated value long 64 characters
- **file path** `std::string` converted in `std::filesystem::path` -> path to interested file (server will check path validity against its file system)

**RESPONSE**: **1** file correctly deleted, **0** no changes to the server filesystem.

## INFO_REQUEST
Used by the client to receive information from the server if the requested file is already present in the server file system with the same name and the same content

**USAGE**:     `INFO_REQUEST | | clientID | | file path | | hashed file`

- **INFO_REQUEST** `int` -> value **2**
- **clientID** `std::string` -> previously saved random generated value long 64 characters
- **file path** `std::string` converted in `std::filesystem::path` -> path to interested file (server will check path validity against its file system)
- **hashed file** `std::string` -> file content hashed with md5 128bit hashing function, in total 32 characters

**RESPONSE**: see info_response.

## INFO_RESPONSE
Used by the server to communicate to a requesting client that the passed file is present or not in its filesystem with the same name and the same content.

**USAGE**:     `INFO_RESPONSE | | clientID | | response`

- **INFO_RESPONSE** `int` -> value **3**
- **clientID** `std::string` -> previously saved random generated value long 64 characters
- **response** `int` -> **1** file is present in server filesytem, **0** file is not present

**RESPONSE**: none.

## END_INFO_PHASE
Used by the client to communicate to the server that it has finished aligning its file system. The final option allows you to force the server to align its file system with that of the client by removing the extra files.

**USAGE**:     `END_INFO_PHASE | | clientID || force align `

- **END_INFO_PHASE** `int` -> value **4**
- **clientID** `std::string` -> previously saved random generated value long 64 characters
- **force align** `int` -> **1** server client folder forced to align to client local folder, **0** server alignment not forced

**RESPONSE**: none.

## LOGIN_REQUEST
Used by the client to authenticate with the server using the client username and the encrypted password as a key. If the client is the first time communicating with the server, this will generate a new clientID for him, a new personal folder in his file system and will add him to the list of users by saving the username and password pair.

**USAGE**:     `LOGIN_REQUEST | | username | | encrypted password `

- **LOGIN_REQUEST** `int` -> value **5**
- **username** `std::string` -> client username
- **encrypted password** `std::string` -> client password

**RESPONSE**: see login_response.

## LOGIN RESPONSE

Used by the server to communicate the result of the login request to the client and it's clientID

**USAGE**:     `LOGIN RESPONSE | | clientID | | response `

- **LOGIN RESPONSE** `int` -> value **6**
- **clientID** `std::string` -> previously saved random generated value long 64 characters
- **response** `int` -> **1** client correctly authenticated, **0** invalid password

**RESPONSE**: none.
