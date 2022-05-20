#import socket module
from socket import *
# In order to terminate the program
import sys 
# In order to use realpath
import os

serverSocket = socket(AF_INET, SOCK_STREAM)

#Prepare a sever socket
serverPort = 25566
serverSocket.bind(('', serverPort))
serverSocket.listen(1)

while True:
  #Establish the connection
  print('Ready to serve...')
  connectionSocket, addr = serverSocket.accept()
  try:
    message = connectionSocket.recv(1024).decode()
    message = message.split()

    print("Got request: ")
    print(message)
    
    if len(message) == 0:
      continue

    filename = message[1][1:]
    code_path = os.path.realpath(__file__)
    file_path = code_path[:code_path.rfind("/")] + "/" + filename
    
    f = open(file_path, "rb")

    #Send one HTTP header line into socket
    connectionSocket.send("HTTP/1.1 200 OK".encode());

    #Send the content of the requested file to the client
    byte = f.read(1024)
    while(byte):
      connectionSocket.send(byte)
      byte = f.read(1024)
    connectionSocket.send("\r\n".encode())

    connectionSocket.close()
  except IOError:
    print('error')
    #Send response message for file not found
    connectionSocket.send("HTTP/1.1 404 Not Found".encode());

    #Close client socket
    connectionSocket.close()

serverSocket.close()
sys.exit()  #Terminate the program after sending the corresponding data