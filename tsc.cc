#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include <string>
#include <unistd.h>
#include <csignal>
#include <grpc++/grpc++.h>
#include "client.h"

#include "sns.grpc.pb.h"
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using csce438::Message;
using csce438::ListReply;
using csce438::Request;
using csce438::Reply;
using csce438::SNSService;
using namespace std;
void sig_ignore(int sig) {
  std::cout << "Signal caught " + sig;
}

Message MakeMessage(const std::string& username, const std::string& msg) {
    Message m;
    m.set_username(username);
    m.set_msg(msg);
    google::protobuf::Timestamp* timestamp = new google::protobuf::Timestamp();
    timestamp->set_seconds(time(NULL));
    timestamp->set_nanos(0);
    m.set_allocated_timestamp(timestamp);
    return m;
}


class Client : public IClient
{
public:
  Client(const std::string& hname,
	 const std::string& uname,
	 const std::string& p)
    :hostname(hname), username(uname), port(p) {}

  
protected:
  virtual int connectTo();
  virtual IReply processCommand(std::string& input);
  virtual void processTimeline();

private:
  std::string hostname;
  std::string username;
  std::string port;
  
  // You can have an instance of the client stub
  // as a member variable.
  std::unique_ptr<SNSService::Stub> stub_;
  
  IReply Login();
  IReply List();
  IReply Follow(const std::string &username);
  IReply UnFollow(const std::string &username);
  void   Timeline(const std::string &username);
};


///////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////
int Client::connectTo()
{
  // ------------------------------------------------------------
  // In this function, you are supposed to create a stub so that
  // you call service methods in the processCommand/porcessTimeline
  // functions. That is, the stub should be accessible when you want
  // to call any service methods in those functions.
  // Please refer to gRpc tutorial how to create a stub.
  // ------------------------------------------------------------
    
///////////////////////////////////////////////////////////
// YOUR CODE HERE
//////////////////////////////////////////////////////////
  stub_ =  csce438::SNSService::NewStub(grpc::CreateChannel(hostname+":"+port, grpc::InsecureChannelCredentials()));
  IReply reply = Login();
  if (reply.comm_status != SUCCESS) {
    return -1;
  }
  return 1;
}

IReply Client::processCommand(std::string& input)
{
  // ------------------------------------------------------------
  // GUIDE 1:
  // In this function, you are supposed to parse the given input
  // command and create your own message so that you call an 
  // appropriate service method. The input command will be one
  // of the followings:
  //
  // FOLLOW <username>
  // UNFOLLOW <username>
  // LIST
  // TIMELINE
  // ------------------------------------------------------------
  
  // ------------------------------------------------------------
  // GUIDE 2:
  // Then, you should create a variable of IReply structure
  // provided by the client.h and initialize it according to
  // the result. Finally you can finish this function by returning
  // the IReply.
  // ------------------------------------------------------------
  
  
  // ------------------------------------------------------------
  // HINT: How to set the IReply?
  // Suppose you have "FOLLOW" service method for FOLLOW command,
  // IReply can be set as follow:
  // 
  //     // some codes for creating/initializing parameters for
  //     // service method
  //     IReply ire;
  //     grpc::Status status = stub_->FOLLOW(&context, /* some parameters */);
  //     ire.grpc_status = status;
  //     if (status.ok()) {
  //         ire.comm_status = SUCCESS;
  //     } else {
  //         ire.comm_status = FAILURE_NOT_EXISTS;
  //     }
  //      
  //      return ire;
  // 
  // IMPORTANT: 
  // For the command "LIST", you should set both "all_users" and 
  // "following_users" member variable of IReply.
  // ------------------------------------------------------------

    IReply ire;
    int space_index = input.find(' ');
    if (space_index != std::string::npos) {
      std::string cmd = input.substr(0, space_index);
      std::string arg = input.substr(space_index+1);
      if (cmd == "FOLLOW") {
        ire = Follow(arg);
      } else if (cmd == "UNFOLLOW") {
        ire = UnFollow(arg);
      }
    } else if (input == "LIST") {
      ire = List();
    } else if (input == "TIMELINE") {
      Timeline(username);
    }

    /*********
    YOUR CODE HERE
    **********/

    return ire;
}


void Client::processTimeline()
{
    Timeline(username);
}

// List Command
IReply Client::List() {

    IReply ire;

    /*********
    YOUR CODE HERE
    **********/
    ClientContext cc;
    ListReply reply;
    Request req;
    req.set_username(username);
    Status status = stub_->List(&cc, req, &reply);
    if (status.ok()) {
      ire.comm_status = IStatus::SUCCESS;
      for (auto all : reply.all_users()) {
        ire.all_users.push_back(all);
      }
      for (auto all : reply.followers()) {
        ire.followers.push_back(all);
      }
    } else {
      ire.comm_status = IStatus::FAILURE_UNKNOWN;
    }
    return ire;
}

// Follow Command        
IReply Client::Follow(const std::string& username2) {

    IReply ire; 
    Request req;
    req.set_username(username);
    req.add_arguments(username2);
    /***
    YOUR CODE HERE
    ***/
    Reply reply;
    ClientContext cc;
    if (username2 == username) {
      ire.comm_status = IStatus::FAILURE_ALREADY_EXISTS;
    } else {
      Status status = stub_->Follow(&cc, req, &reply);
      if (reply.msg() == "already follow user") {
        ire.comm_status = IStatus::FAILURE_ALREADY_EXISTS;
      } else if (reply.msg() == "invalid username") {
        ire.comm_status = IStatus::FAILURE_INVALID_USERNAME;
      } else if (reply.msg() == "successfully followed") {
        ire.comm_status = IStatus::SUCCESS;
      } else {
        ire.comm_status = IStatus::FAILURE_UNKNOWN;
      }
    }
    return ire;
}

// UNFollow Command  
IReply Client::UnFollow(const std::string& username2) {
    IReply ire; 
    Request req;
    req.set_username(username);
    req.add_arguments(username2);
    /***
    YOUR CODE HERE
    ***/
    Reply reply;
    ClientContext cc;
    if (username == username2) {
      ire.comm_status = IStatus::FAILURE_INVALID_USERNAME;
    } else {
      Status status = stub_->UnFollow(&cc, req, &reply);
      if (reply.msg() == "you are not a follower") {
        ire.comm_status = IStatus::FAILURE_NOT_A_FOLLOWER;
      } else if (reply.msg() == "unfollow successful") {
        ire.comm_status = IStatus::SUCCESS;
      } else if (reply.msg() == "not a valid user") {
        ire.comm_status = IStatus::FAILURE_INVALID_USERNAME;
      } else {
        ire.comm_status = IStatus::FAILURE_UNKNOWN;
      }
    }
    return ire;
}

// Login Command  
IReply Client::Login() {

    IReply ire;

    Request request;
    Reply reply;
    ClientContext context;

    request.set_username(username);
    Status status = stub_->Login(&context, request, &reply);

    ire.grpc_status = status;
    if (reply.msg() == "logged in"){
      ire.comm_status = IStatus::SUCCESS;
    } else{
      ire.comm_status = IStatus::FAILURE_ALREADY_EXISTS;
    }
    return ire;
    /***
     YOUR CODE HERE
    ***/
    
}

// Timeline Command
void Client::Timeline(const std::string& username) {

    // ------------------------------------------------------------
    // In this function, you are supposed to get into timeline mode.
    // You may need to call a service method to communicate with
    // the server. Use getPostMessage/displayPostMessage functions 
    // in client.cc file for both getting and displaying messages 
    // in timeline mode.
    // ------------------------------------------------------------

    // ------------------------------------------------------------
    // IMPORTANT NOTICE:
    //
    // Once a user enter to timeline mode , there is no way
    // to command mode. You don't have to worry about this situation,
    // and you can terminate the client program by pressing
    // CTRL-C (SIGINT)
    // ------------------------------------------------------------
  
    /***
    YOUR CODE HERE
    ***/
    ClientContext context;
    shared_ptr<ClientReaderWriter<Message, Message>> stream(stub_->Timeline(&context));
    
    thread writethread([username, stream](){
        string start = "CSCE 438";
        Message m = MakeMessage(username, start);
        stream->Write(m);
        while(true){
          start = getPostMessage();
          m = MakeMessage(username, start);
          stream->Write(m);
        }
        stream->WritesDone();
    });
    
    
    thread readthread([username, stream](){
        Message m;
        while(stream->Read(&m)){
            google::protobuf::Timestamp timestamp = m.timestamp();
            time_t t = timestamp.seconds();
            displayPostMessage(m.username(), m.msg(), t);
        }
    });
	
    writethread.join();
    readthread.join();    
	
    
}



//////////////////////////////////////////////
// Main Function
/////////////////////////////////////////////
int main(int argc, char** argv) {

  std::string hostname = "localhost";
  std::string username = "default";
  std::string port = "3010";
    
  int opt = 0;
  while ((opt = getopt(argc, argv, "h:u:p:")) != -1){
    switch(opt) {
    case 'h':
      hostname = optarg;break;
    case 'u':
      username = optarg;break;
    case 'p':
      port = optarg;break;
    default:
      std::cout << "Invalid Command Line Argument\n";
    }
  }
      
  std::cout << "Logging Initialized. Client starting...";
  
  Client myc(hostname, username, port);
  
  myc.run();
  
  return 0;
}
