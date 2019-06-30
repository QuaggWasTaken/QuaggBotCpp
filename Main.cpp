#include "sleepy_discord/websocketpp_websocket.h"
#include <cpprest/http_client.h>
#include <iostream>
#include <fstream>
#include <cpprest/filestream.h>
#include "msxml.h"
#include <sstream>
#include <ctime>    // For time()
#include <cstdlib>  // For srand() and rand()
#include <math.h>

#include "tinyxml2.h"
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ctime>



int gPass = 0;
int gFail = 0;

using namespace tinyxml2;
using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using namespace web::http;                  // Common HTTP functionality
using namespace web::http::client;          // HTTP client features
using namespace concurrency::streams;       // Asynchronous streams

//Space reserved for function declarations
std::string getCommand(SleepyDiscord::Message);
void help(SleepyDiscord::Message);
void echo(SleepyDiscord::Message);
void loadXML(SleepyDiscord::Message);
void getDefine(SleepyDiscord::Message);
void rollDice(SleepyDiscord::Message);
std::string mwKey;
std::string problemWords = "test, fat";

class MyClientClass : public SleepyDiscord::DiscordClient {
public:

	void NullLineEndings(char* p)
	{
		while (p && *p) {
			if (*p == '\n' || *p == '\r') {
				*p = 0;
				return;
			}
			++p;
		}
	}

	using SleepyDiscord::DiscordClient::DiscordClient;

	void onMessage(SleepyDiscord::Message message) override {
		if (message.author.username != "QuaggBot") {
			if (message.attachments.empty()) {
				std::string command = getCommand(message);
				if (message.startsWith("<>")) {
					if (command == "help") {
						help(message);
					}
					else if (command == "ping") {
						sendMessage(message.channelID, "Pong");
					}
					else if (command == "oof") {
						sendMessage(message.channelID, "oof ouch owie my bones");
					}
					else if (command == "hi" || command == "hello") {
						if (message.author.ID != "103977057024761856") {
							sendMessage(message.channelID, "Hello " + message.author.username);
						}
						else {
							sendMessage(message.channelID, "Hello creator");
						}
					}
					else if (command == "echo") {
						echo(message);
					}
					else if (command == "define") {
						//loadXML(message);
						sendMessage(message.channelID, "Sorry, this command is disabled because of bugs causing crashes, and will be re-enabled upon fix.");
					}
					else if (command == "roll") {
						rollDice(message);
					}
					else if (command == "dm") {
						dm(message, "Hello! This is a DM!");
					}
					else if (command == "calc") {
						calculate(message);
					}
					else {
						sendMessage(message.channelID, "Sorry, that's not a valid command, please check <>help for a list of valid commands");
					}
				}
			}
		}
	}

	void help(SleepyDiscord::Message message) {
		std::string helpMessage = "Full documentation can be found at https://github.com/QuaggWasTaken/QuaggBotCpp \\n\\n***Commands:***\\n*All commands prefixed with <>*\\n\\nhelp: Displays this message\\n\\nping: pong\\n\\noof: ouch owie my bones\\n\\nhi: Says Hello\\n\\ndefine[word]: Defines whatever word you replace the brackets with.For example, <>define help returns the definition for help (This is currently disabled, as there were too many bugs and glitches causing crashes)\\n\\nroll: Rolls specified dice.For Example, <>roll 1d4 rolls 1 4 sided dice\\n\\ncalc: Simple calculator. Adds, subtracts, multiplies, or divides two numbers. <>calc 2*4 returns 8";
		dm(message, helpMessage);
		sendMessage(message.channelID, "Help message sent to DM!");
	}

	std::string getCommand(SleepyDiscord::Message message) {
		std::string command;
		std::string content = message.content;
		if (content.find(" ") != std::string::npos) {
			command = content.substr(2, content.find(" ") - 2);
		}
		else {
			command = content.substr(2);
		}
		std::cout << command << std::endl;
		return command;
	}

	void echo(SleepyDiscord::Message message) {
		int commandIndex = getCommand(message).length() + 2;
		std::string text;
		std::string input = message.content;
		text = input.substr(commandIndex);
		sendMessage(message.channelID, text);
	}

	void getDefine(SleepyDiscord::Message message) {

		//SOMETHING HERE ERRORS WHEN RECIEVING A BAD XML FILE, MUST FIX EVENTUALLY
		std::string results;
		std::string defStr;
		tinyxml2::XMLDocument doc;
		doc.LoadFile("results.xml");

		tinyxml2::XMLElement* defElement = doc.FirstChildElement()->FirstChildElement()->FirstChildElement("def")->FirstChildElement("dt");
		const char* def = defElement->GetText();
		defStr = def;
		results = defStr.substr((defStr.find(":")) + 1, (defStr.find(":", (defStr.find(":")) + 1)) - 2);
		sendMessage(message.channelID, results);

	}

	void loadXML(SleepyDiscord::Message message) {

		auto fileStream = std::make_shared<ostream>();

		// Open stream to output file.
		pplx::task<void> requestTask = fstream::open_ostream(U("results.xml")).then([=](ostream outFile)
		{
			*fileStream = outFile;

			// Create http_client to send the request.
			http_client client(U("https://dictionaryapi.com"));

			// Build request URI and start the request.
			uri_builder builder(U("/api/v1/references/collegiate/xml/"));
			builder.append_path(conversions::to_string_t(message.content.substr(9)));
			builder.append_query(U("key"), conversions::to_string_t(mwKey));
			return client.request(methods::GET, builder.to_string());
		})

			// Handle response headers arriving.
			.then([=](http_response response)
		{
			printf("Received response status code:%u\n", response.status_code());

			// Write response body into the file.
			return response.body().read_to_end(fileStream->streambuf());
		})
			// Close the file stream.
			.then([=](size_t)
		{
			return fileStream->close();
		});

		// Wait for all the outstanding I/O to complete and handle any exceptions
		try
		{
			requestTask.wait();
		}
		catch (const std::exception &e)
		{
			printf("Error exception:%s\n", e.what());
			sendMessage(message.channelID, "That's not a word according to Merriam Webster, try again with a real word");
			return;
		}
		getDefine(message);
		return;
	}
	void rollDice(SleepyDiscord::Message message) {
		//Define vars
		std::string numDiceStr;
		int numDice;
		std::string diceSizeStr;
		int diceSize;
		std::string input;
		srand(time(0));			//Starts random number generator using system time as seed
		int rollTotal = 0;
		std::string output = "";
		//Parse input for vars
		input = message.content.substr(7);
		//std::cout << input;
		numDiceStr = input.substr(0, input.find("d"));
		diceSizeStr = input.substr(input.find("d") + 1);
		{						//Assigns numDice, can error and exit loop
			std::istringstream stoi(numDiceStr);
			stoi >> numDice;
			if (stoi.fail()) {
				std::cout << numDiceStr;
				sendMessage(message.channelID, "That's not a valid input. Please use the syntax idx, with i being the number of dice and x being how many sides on each dice.");
				return;
			}
		}
		{						//Assigns diceSize, can error and exit loop
			std::istringstream stoi(diceSizeStr);
			stoi >> diceSize;
			if (stoi.fail()) {
				std::cout << diceSizeStr;
				sendMessage(message.channelID, "That's not a valid input. Please use the syntax idx, with i being the number of dice and x being how many sides on each dice.");
				return;
			}
		}
		for (int i = 0; i < numDice; i++) {
			int currRoll = (rand() % diceSize) + 1;
			rollTotal += currRoll;
			std::cout << rollTotal << std::endl;

			if (i == 0) {
				output += std::to_string(currRoll);
			}
			else {
				output += (", " + std::to_string(currRoll));
			}
		}
		output += "\\nYou rolled a total of: " + std::to_string(rollTotal);
		std::cout << output << std::endl;
		sendMessage(message.channelID, output);
	}
	void calculate(SleepyDiscord::Message message) {
		std::string input = message.content.substr(6);
		std::string op = "";
		std::string::size_type sz;

		double num1 = std::stod(input, &sz);
		double num2 = std::stod(input.substr(sz + 1));
		op = input.substr(sz, 1);
		float result;
		if (op == "+") {
			result = num1 + num2;
		}
		else if (op == "-") {
			result = num1 - num2;
		}
		else if (op == "*") {
			result = num1 * num2;
		}
		else if (op == "/") {
			result = num1 / num2;
		}

		sendMessage(message.channelID, std::to_string(result));
	}
	void dm(SleepyDiscord::Message message, std::string content) {
		SleepyDiscord::DMChannel DMChan;
		DMChan = createDirectMessageChannel(message.author.ID.string());
		SleepyDiscord::Snowflake<SleepyDiscord::Channel> chan;
		sendMessage((SleepyDiscord::Snowflake<SleepyDiscord::Channel>) DMChan.ID, content);
	}

};

int main() {

	//SOMETHING HERE ERRORS WHEN RECIEVING A BAD XML FILE, MUST FIX EVENTUALLY
	std::string results;
	std::string defStr;
	tinyxml2::XMLDocument doc;
	doc.LoadFile("keys.xml");

	tinyxml2::XMLElement* disKeyElement = doc.FirstChildElement()->FirstChildElement("disKey");
	const char* tokenConst = disKeyElement->GetText();
	tinyxml2::XMLElement* mwKeyElement = doc.FirstChildElement()->FirstChildElement("mwKey");
	mwKey = mwKeyElement->GetText();

	//std::string token;
	std::cout << "Please input Discord token:" << std::endl;
	//std::cin >> token;
	std::cout << "\nPlease input MW API Key:" << std::endl;
	//std::cin >> mwKey;
	//std::cout << "test";
	MyClientClass client(tokenConst, 2);
	client.run();
}