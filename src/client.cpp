#include <client.hpp>
#include <server.hpp>
#include <type_traits>


namespace MCPP {


	static const String pa_banner("====PROTOCOL ANALYSIS====");
	static const String send_pa_template("Server ==> {0}:{1} - Packet of type {2}");
	static const String encrypt_banner("====ENCRYPTION ENABLED====");
	static const String key_pa("Encryption enabled with key:");
	static const String iv_pa("Encryption enabled with IV:");
	static const String recv_pa_template("{0}:{1} ==> Server - Packet of type {2}");
	
	
	//	Formats a byte for display/logging
	static inline String byte_format (Byte b) {
	
		String returnthis("0x");
		
		String byte(b,16);
		
		if (byte.Count()==1) returnthis << "0";
		
		returnthis << byte;
		
		return returnthis;
	
	}
	
	
	//	Formats a buffer of bytes for
	//	display/logging
	static inline String buffer_format (const Vector<Byte> & buffer) {
	
		String returnthis;
	
		//	Print each byte is hexadecimal
		//	format
		for (Word i=0;i<buffer.Count();++i) {
		
			//	We don't need a space
			//	or a newline before
			//	the very first byte
			if (i!=0) {
			
				//	End of line
				if ((i%8)==0) returnthis << Newline;
				//	Space before each byte
				//	that isn't the first or
				//	right after a newline
				else returnthis << " ";
			
			}
			
			returnthis << byte_format(buffer[i]);
		
		}
		
		return returnthis;
	
	}


	Client::Client (SmartPointer<Connection> conn) noexcept
		:	conn(std::move(conn)),
			packet_in_progress(false),
			packet_encrypted(false)
	{
	
		state=static_cast<Word>(ClientState::Connected);
	
	}
	
	
	void Client::EnableEncryption (const Vector<Byte> & key, const Vector<Byte> & iv) {
	
		//	Protocol analysis
		if (RunningServer->ProtocolAnalysis) {
		
			String log(pa_banner);
			log	<<	Newline
				<<	key_pa
				<<	Newline
				<<	buffer_format(key)
				<<	Newline
				<<	iv_pa
				<<	Newline
				<<	buffer_format(iv);
				
			RunningServer->WriteLog(
				log,
				Service::LogType::Information
			);
		
		}
	
		comm_lock.Write([&] () {	encryptor.Construct(key,iv);	});
	
	}
	
	
	SmartPointer<SendHandle> Client::Send (const Packet & packet) {
	
		//	Protocol analysis
		if (RunningServer->ProtocolAnalysis) {
		
			String log(pa_banner);
			log << Newline << String::Format(
				send_pa_template,
				conn->IP(),
				conn->Port(),
				byte_format(packet.Type())
			) << Newline << packet.ToString();
			
			RunningServer->WriteLog(
				log,
				Service::LogType::Information
			);
		
		}
	
		//	Lock
		return comm_lock.Read([&] () {
		
			//	Send
			return conn->Send(
				//	Is encryption on?
				encryptor.IsNull()
						//	No, send plaintext
					?	packet.ToBytes()
						//	Yes, send ciphertext
					:	encryptor->Encrypt(
							packet.ToBytes()
						)
			);
		
		});
	
	}
	
	
	SmartPointer<SendHandle> Client::Send (const Packet & packet, const Vector<Byte> & key, const Vector<Byte> & iv) {
	
		//	Protocol Analysis
		if (RunningServer->ProtocolAnalysis) {
		
			String log(pa_banner);
			log	<< 	Newline
				<< 	String::Format(
						send_pa_template,
						conn->IP(),
						conn->Port(),
						byte_format(packet.Type())
					)
				<<	Newline
				<<	packet.ToString()
				<<	Newline
				<<	encrypt_banner
				<<	Newline
				<<	key_pa
				<<	Newline
				<<	buffer_format(key)
				<<	Newline
				<<	iv_pa
				<<	Newline
				<<	buffer_format(iv);
				
			RunningServer->WriteLog(
				log,
				Service::LogType::Information
			);
		
		}
	
		//	Lock
		return comm_lock.Write([&] () {
		
			//	If encryption is not enabled,
			//	send plaintext and then
			//	enable it
			if (encryptor.IsNull()) {
			
				//	Plaintext send
				auto handle=conn->Send(
					packet.ToBytes()
				);
				
				//	Enable encryption
				encryptor.Construct(key,iv);
				
				SetState(ClientState::Authenticated);
				
				//	Return send handle
				return handle;
			
			}
			
			//	If encryption is already enabled,
			//	ignore the spurious request to
			//	enable it, and perform an encrypted
			//	send
			return conn->Send(
				encryptor->Encrypt(
					packet.ToBytes()
				)
			);
		
		});
	
	}
	
	
	void Client::Disconnect () noexcept {
	
		conn->Disconnect();
	
	}
	
	
	void Client::Disconnect (const String & reason) noexcept {
	
		conn->Disconnect(reason);
	
	}
	
	
	void Client::Disconnect (String && reason) noexcept {
	
		conn->Disconnect(std::move(reason));
	
	}
	
	
	bool Client::Receive (Vector<Byte> * buffer) {
	
		//	Check for null
		if (buffer==nullptr) throw std::out_of_range(NullPointerError);
		
		//	Short-circuit out of the buffer
		//	happens to be empty
		if (buffer->Count()==0) return false;
		
		//	This is activity
		Active();
		
		//	Acquire lock so encryption
		//	state doesn't change
		return comm_lock.Read([&] () {
		
			//	There's no encryption if...
			if (
				(
					//	A packet is in progress and...
					packet_in_progress &&
					//	...that packet started out unencrypted
					!packet_encrypted
				//	or
				) ||
				//	If there's no encryption provider
				encryptor.IsNull()
			) {
			
				//	No encryption
				packet_encrypted=false;
				
				//	Preserve count so we can
				//	determine whether the packet
				//	started constructing itself
				Word before=buffer->Count();
				
				bool complete=in_progress.FromBytes(*buffer);
				
				//	Flag packet as in progress
				//	if necessary
				if (
					complete ||
					before!=buffer->Count()
				) packet_in_progress=true;
				
				return complete;
			
			}
			
			//	Encryption
			
			//	Decrypt directly into decryption buffer
			encryptor->Decrypt(buffer,&encryption_buffer);
			
			//	Attempt to extract a packet
			return in_progress.FromBytes(encryption_buffer);
		
		});
	
	}
	
	
	Packet Client::CompleteReceive () noexcept {
	
		//	Protocol Analysis
		if (RunningServer->ProtocolAnalysis) {
		
			String log(pa_banner);
			log << Newline << String::Format(
				recv_pa_template,
				conn->IP(),
				conn->Port(),
				byte_format(in_progress.Type())
			) << Newline << in_progress.ToString();
			
			RunningServer->WriteLog(
				log,
				Service::LogType::Information
			);
		
		}
	
		//	Reset in progress flag
		packet_in_progress=false;
	
		return std::move(in_progress);
	
	}
	
	
	void Client::SetState (ClientState state) noexcept {
	
		this->state=static_cast<Word>(state);
	
	}
	
	
	ClientState Client::GetState () const noexcept {
	
		return static_cast<ClientState>(
			static_cast<Word>(state)
		);
	
	}
	
	
	void Client::SetUsername (String username) noexcept {
	
		username_lock.Execute([&] () {	this->username=std::move(username);	});
	
	}
	
	
	String Client::GetUsername () const {
	
		return username_lock.Execute([&] () {	return username;	});
	
	}
	
	
	const Connection * Client::GetConn () const noexcept {
	
		return static_cast<const Connection *>(conn);
	
	}
	
	
	const IPAddress & Client::IP () const noexcept {
	
		return conn->IP();
	
	}
	
	
	UInt16 Client::Port () const noexcept {
	
		return conn->Port();
	
	}
	
	
	void Client::Active () {
	
		inactive_lock.Execute([&] () {	inactive.Reset();	});
	
	}
	
	
	Word Client::Inactive () const {
	
		return inactive_lock.Execute([&] () {	return inactive.ElapsedMilliseconds();	});
	
	}


}
