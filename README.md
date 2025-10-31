# sistemas-distribuidos
## Laboratory 1
Simple tx and rx with lora using an esp32 Heltec v3 between and admin and user, the admin register users and ask if they are available, sending a message for every user and every user has to answer depending on id assigned.

## Laboratory 2
### Keys
Admin node register keys in another node which serves as a database, 
using xor encryption.
### Chat
Admin and user can interchange messages each one using xor encryption
### Pipeline
Through the terminal we execute 2 files, 1 is printing data in the terminal, 2 is the code in python which gets that information with Serial and plot the points with matplotlib.
### Http
Importing request with python, we fetch data from jsonplaceholder website, the title and body is recovered, when prg button is pressed once, the title should be printed, when twice, body should be printed.
### Chat Escalated
Admin can have as much as users are registered and following structure header-body, admin can talk individually with an specific user.

## CANSAT
We using angular, mongodb, redis, implementing an Layers Architecture, the cansat send data through Serial, an d a publisher with redis is charged of publish it in mongodb, this reduces latency and increases speed of transmition, then with mongo, we fetch in the front to show the information of the launch made.
