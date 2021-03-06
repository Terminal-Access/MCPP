#include <rleahylib/rleahylib.hpp>
#include <info/info.hpp>
#include <client.hpp>
#include <mod.hpp>
#include <server.hpp>


using namespace MCPP;


static const String identifier("clients");
static const String help("Displays information about each connected client.");
static const String clients_banner("CONNECTED CLIENTS:");
static const String client_template("{0}:{1}");
static const String client_authenticated("Authenticated");
static const String client_username("Username");
static const String client_connected("Connected For");
static const String client_sent("Bytes Sent");
static const String client_received("Bytes Received");
static const String client_latency_template("{0}ms");
static const String client_latency("Latency");
static const String username_template(" ({0})");
static const String yes("Yes");
static const String no("No");
static const String info_separator(", ");
static const String name("Client Information");
static const Word priority=1;


static inline String zero_pad (String pad, Word num) {

	while (pad.Count()<num) pad=String("0")+pad;
	
	return pad;

}


static inline String connected_format (Word milliseconds) {

	String time;
	time	<<	zero_pad(
					String(milliseconds/(1000*60*60)),
					2
				)
			<<	":"
			<<	zero_pad(
					String((milliseconds%(1000*60*60))/(1000*60)),
					2
				)
			<<	":"
			<<	zero_pad(
					String((milliseconds%(1000*60))/1000),
					2
				)
			<<	"."
			<<	zero_pad(
					String(milliseconds%1000),
					3
				);
				
	return time;

}


class ClientsInfo : public Module, public InformationProvider {


	public:
	
	
		virtual const String & Identifier () const noexcept override {
		
			return identifier;
		
		}
		
		
		virtual const String & Help () const noexcept override {
		
			return help;
		
		}
		
		
		virtual void Execute (ChatMessage & message) const override {
		
			message	<< ChatStyle::Bold
					<< clients_banner
					<< ChatFormat::Pop
					<< ChatFormat::Pop;
					
			for (auto & client : Server::Get().Clients) {
			
				String username;
				if (client->GetState()==ProtocolState::Play) username=String::Format(
					username_template,
					client->GetUsername()
				);
			
				message	<<	Newline
						<<	ChatStyle::Bold
						<<	String::Format(
								client_template,
								client->IP(),
								client->Port()
							)
						<<	username
						<<	": "
						<<	ChatFormat::Pop
						//	Authenticated?
						<<	client_authenticated
						<<	": "
						<<	((client->GetState()==ProtocolState::Play) ? yes : no)
						<<	info_separator
						//	Time connected
						<<	client_connected
						<<	": "
						<<	connected_format(client->Connected())
						<<	info_separator
						//	Latency
						<<	client_latency
						<<	": "
						<<	String::Format(
								client_latency_template,
								static_cast<Word>(client->Ping)
							)
						<<	info_separator
						//	Bytes sent
						<<	client_sent
						<<	": "
						<<	client->Sent()
						<<	info_separator
						//	Bytes received
						<<	client_received
						<<	": "
						<<	client->Received();
			
			}
		
		}
		
		
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual void Install () override {
		
			Information::Get().Add(this);
		
		}


};


INSTALL_MODULE(ClientsInfo)
