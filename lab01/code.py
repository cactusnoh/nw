#import socket module
from socket import *
# In order to terminate the program
import sys 
import os

serverSocket = socket(AF_INET, SOCK_STREAM)

#Prepare a sever socket
serverPort = 25567
serverSocket.bind(('', serverPort))
serverSocket.listen(1)

while True:
  #Establish the connection
  print('Ready to serve...')
  connectionSocket, addr = serverSocket.accept()
  try:
    message = connectionSocket.recv(1024).decode()
    message = message.split()

    filename = message[1][1:]
    code_path = os.path.realpath(__file__)
    file_path = code_path[:code_path.rfind("/")] + "/" + filename

    print("Got request: " + message[0] + " " + file_path)
    
    f = open(file_path)
    outputdata = f.readlines()

    #Send one HTTP header line into socket
    connectionSocket.send("HTTP/1.1 200 OK".encode());

    #Send the content of the requested file to the client
    for i in range(0, len(outputdata)):
      connectionSocket.send(outputdata[i].encode())
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