#include <unordered_map>
#include <algorithm>
#include "generator/generator.h"
#include "json.hpp"
#include "crow_all.h"
#include "restclient-cpp/connection.h"
#include "restclient-cpp/restclient.h"

using json = nlohmann::json;

const std::string ENDPOINT = "https://graph.facebook.com/v2.6/me/messages?access_token=EAACaA7moyIUBANyaZCyAmQpZCeLoPViwS46dniLZCMSU2bU1ZC3SvWcKrfMECUwPvLy7ZBZBVSfI6iiRsL9ZCYKZB3Gi77ZAlZATU2ZAjSZA5XsdWZBGJv1Bynkpb7Eej7hLEjLHCrbmOoU6EkZCZC2a03TI2pjQZCm45QKGKkUKhYmYHm8I4iSs0660ZBSqi";

std::unordered_map<std::string, int> userStates;
std::unordered_map<std::string, problem> userProblems;
generator mgenerator;

void sendReply(const std::string& sender, const std::string& reply) {
    json j = {
        {"messaging_type", "response"},
        {"recipient", {
            {"id", sender}
        }},
        {"message",{
            {"text", reply}
        }}
    };
    RestClient::post(ENDPOINT, "application/json", j.dump());
}

std::string getReply(const std::string& sender, const std::string& text) {
    if (text == "/play") {
        userStates[sender] = 1;
        problem prob = mgenerator.get_problem();
        userProblems[sender] = prob;
        std::cout << prob.get_ground() << std::endl;
        std::cout << prob.get_scrambled() << std::endl;
        return prob.get_scrambled();
    }

    int userState = userStates[sender];
    if (userState == 0) {
        return "Hello.. Please type '/play' to start playing..";
    } else if (userState == 1) {
        problem prob = userProblems[sender];
        if (prob.check(text)) {
            // correct
            userStates[sender] = 0;
            return "Correct! Congratulations!";
        } else {
            // wrong
            return "Wrong.. Please try again..";
        }
    }
}

void respondMessage(const json& j) {
    if (!j.count("entry")) return;
    json entryArr = j["entry"];
    if (!entryArr.is_array()) return;
    json entry = entryArr[0];
    if (!entry.count("messaging")) return;
    json messagingArr = entry["messaging"];
    if (!messagingArr.is_array()) return;
    json messaging = messagingArr[0];
    if (!messaging.count("sender")) return;
    json sender = messaging["sender"];
    if (!sender.count("id")) return;
    if (!messaging.count("message")) return;
    json message = messaging["message"];
    if (!message.count("text")) return;
    std::cout << "text: " << message["text"] << std::endl;
    std::string reply = getReply(sender["id"], message["text"]);
    sendReply(sender["id"], reply);
}

crow::response handleGet(const crow::request& req) {
    std::string mode = "";
    if (req.url_params.get("hub.mode") != nullptr) {
        mode = boost::lexical_cast<std::string>(req.url_params.get("hub.mode"));
    }
    std::string challenge = "";
    if (req.url_params.get("hub.challenge") != nullptr) {
        challenge = boost::lexical_cast<std::string>(req.url_params.get("hub.challenge"));
    }
    std::string token = "";
    if (req.url_params.get("hub.verify_token") != nullptr) {
        token = boost::lexical_cast<std::string>(req.url_params.get("hub.verify_token"));
    }
    std::string response;
    if (token == "anagraman") {
        response = challenge;
    } else {
        response = "Invalid Token";
    }
    return crow::response{ response };
}

crow::response handlePost(const crow::request& req) {
    json j = json::parse(req.body);
    respondMessage(j);
    return crow::response(200);
}

int main(int argc, char *argv[]) {
    RestClient::init();
    crow::SimpleApp app;

    CROW_ROUTE(app, "/facebook").methods(crow::HTTPMethod::Get, crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        if (req.method == crow::HTTPMethod::Get) {
            std::cout << "get request from facebook" << std::endl;
            return handleGet(req);
        } else if (req.method == crow::HTTPMethod::Post) {
            std::cout << "receive message from facebook" << std::endl;
            return handlePost(req);
        } else {
            return crow::response(405);
        }
    });

    app.port(9080).multithreaded().run();
    return 0;
}

