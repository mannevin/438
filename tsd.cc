/*
 *
 * Copyright 2015, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <ctime>

#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/duration.pb.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <google/protobuf/util/time_util.h>
#include <grpc++/grpc++.h>
#include<glog/logging.h>
#define log(severity, msg) LOG(severity) << msg; google::FlushLogFiles(google::severity); 

#include "sns.grpc.pb.h"

using namespace std;

using google::protobuf::Timestamp;
using google::protobuf::Duration;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using csce438::Message;
using csce438::ListReply;
using csce438::Request;
using csce438::Reply;
using csce438::SNSService;


struct Client {
  std::string username;
  bool connected = true;
  int following_file_size = 0;
  std::vector<Client*> client_followers;
  std::vector<Client*> client_following;
  ServerReaderWriter<Message, Message>* stream = 0;
  bool operator==(const Client& c1) const{
    return (username == c1.username);
  }
};

//Vector that stores every client that has been created
std::vector<Client*> client_db;
std::unordered_map<Client *, std::vector<Message>> unseenPosts;
string trim(const std::string& str) {
    size_t start = 0;
    while (start < str.length() && (std::isspace(str[start]) || str[start] == '\n' || str[start] == '\t')) {
        start++;
    }
    size_t end = str.length();
    while (end > start && (std::isspace(str[end - 1]) || str[end - 1] == '\n' || str[end - 1] == '\t')) {
        end--;
    }
    return str.substr(start, end - start);
  }
int findUser(vector<Client*> arr, std::string name) {
  for (int i = 0; i < arr.size(); i++) {
    if (arr[i]->username == name) {
      return i;
    }
  }
  return -1;
}
int findIndex(vector<Client*> vec, Client* target) {
    for (size_t i = 0; i < vec.size(); ++i) {
        if (vec[i] == target) {
            return i; 
        }
    }
    return -1;
}
class SNSServiceImpl final : public SNSService::Service {
  
  Status List(ServerContext* context, const Request* request, ListReply* list_reply) override {
    /*********
    YOUR CODE HERE
    **********/
    Client * c1 = client_db[findUser(client_db, request->username())];
    for (auto u : client_db) {
      list_reply->add_all_users(u->username);
    }
    for (auto u : c1->client_followers) {
      list_reply->add_followers(u->username);
    }
    return Status::OK;
  }

  Status Follow(ServerContext* context, const Request* request, Reply* reply) override {

    /*********
    YOUR CODE HERE
    **********/
    string u1 = request->username();
    string u2 = request->arguments(0);
    int ind = findUser(client_db, u2);

    if (ind == -1) {
      reply->set_msg("invalid username");
    } else {
      Client * c1 = client_db[findUser(client_db, u1)];
      Client * c2 = client_db[ind];
      if (findUser(c1->client_following, c2->username) != -1) {
        reply->set_msg("already follow user");
      } else {
        reply->set_msg("successfully followed");
        c1->client_following.push_back(c2);
        c2->client_followers.push_back(c1);
      }
    }
    return Status::OK; 
  }

  Status UnFollow(ServerContext* context, const Request* request, Reply* reply) override {

    /*********
    YOUR CODE HERE
    **********/
    string u1 = request->username();
    string u2 = request->arguments(0);
    int ind = findUser(client_db, u2);
    if (ind == -1) {
      reply->set_msg("not a valid user");
    } else {
      Client * c1 = client_db[findUser(client_db, u1)];
      Client * c2 = client_db[ind];
      if (findUser(c1->client_following, c2->username) != -1) {
        reply->set_msg("unfollow successful");
        c1->client_following.erase(c1->client_following.begin()+findIndex(c1->client_following, c2));
        c2->client_followers.erase(c2->client_followers.begin()+findIndex(c2->client_followers, c1));
      } else {
        reply->set_msg("you are not a follower");
      }
    }
    return Status::OK;
  }

  // RPC Login
  Status Login(ServerContext* context, const Request* request, Reply* reply) override {

    /*********
    YOUR CODE HERE
    **********/
    Client *c = new Client();
    c->username = request->username();
    int pos = findUser(client_db, c->username);
    // for (int i = 0; i < client_db.size(); i++) {
    //   std::cout << client_db[i]->username << std::endl;
    // }
    if (pos == -1) {
      client_db.push_back(c);
      reply->set_msg("logged in");
    } else {
      reply->set_msg("taken username");
    }
    return Status::OK;
  }
  
  Status Timeline(ServerContext* context, ServerReaderWriter<Message, Message>* stream) override {
    // ------------------------------------------------------------
    // In this function, you are to write code that handles 
    // receiving a message/post from a user, recording it in a file
    // and then making it available on his/her follower's streams
    // ------------------------------------------------------------
    Message m;
    while (stream->Read(&m)){

      string username = m.username();
      // find the user that this message is from and get the corresponding fields
      Client * c = client_db[findUser(client_db, username)];
      c->stream = stream;
      google::protobuf::Timestamp timestamp = m.timestamp();
      // create the file format for the message information
      string ffo = m.username() + " (" + google::protobuf::util::TimeUtil::ToString(timestamp) + ") >> " + m.msg();
      // if it is the openeing message, read the last 20 messages from their file
      // if it is not the opening message, save their message to their file
      if (m.msg() != "CSCE 438") {
        ofstream currUserFile(username + ".txt", ofstream::out | ofstream::in | ofstream::app);
        currUserFile << ffo;
      } else {
        ifstream userUnseen(username+"_following.txt");
        int ct = 0;
        string s_message;
        vector<string> messages;
        // read last 20 messages
        while (getline(userUnseen, s_message)) {
          messages.push_back(s_message);
        }
        int startInd;
        if (messages.size() > 20) {
          startInd = messages.size() - 20;
        } else {
          startInd = 0;
        }
        for (int i = messages.size()-1; i >= startInd; i--) {
          Message currMessage;
          currMessage.set_msg(messages[i]);
          stream->Write(currMessage);
        }
      }
      // iterate through the followers of the current user
      for (Client * follower : c->client_followers){
        // if it is the initial stream, dont add the message
        if (m.msg() == "CSCE 438") {
          continue;
        }
        // save the post to the other files
        string otherfilename = follower->username + "_following.txt";
        ofstream other_file(otherfilename, ofstream::out | ofstream::in | ofstream::app);
        if (follower->stream) {
          follower->stream->Write(m);
        }
        other_file << ffo;
      }
    }
    return Status::OK;
  }

};

void RunServer(std::string port_no) {
  std::string server_address = "0.0.0.0:"+port_no;
  SNSServiceImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  log(INFO, "Server listening on "+server_address);

  server->Wait();
}

int main(int argc, char** argv) {

  std::string port = "3010";
  
  int opt = 0;
  while ((opt = getopt(argc, argv, "p:")) != -1){
    switch(opt) {
      case 'p':
          port = optarg;break;
      default:
	  std::cerr << "Invalid Command Line Argument\n";
    }
  }
  
  std::string log_file_name = std::string("server-") + port;
  google::InitGoogleLogging(log_file_name.c_str());
  log(INFO, "Logging Initialized. Server starting...");
  RunServer(port);

  return 0;
}
