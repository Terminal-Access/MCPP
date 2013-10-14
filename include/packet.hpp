/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <json.hpp>
#include <traits.hpp>
#include <cstddef>
#include <cstring>
#include <limits>
#include <new>
#include <type_traits>
#include <utility>


namespace MCPP {


	class BadFormat : public std::exception {
	
	
		public:
		
		
			__attribute__((noreturn))
			static void Raise ();
	
	
	};
	
	
	class InsufficientBytes : public std::exception {
	
	
		public:
		
		
			__attribute__((noreturn))
			static void Raise ();
	
	
	};
	
	
	class BadPacketID : public std::exception {
	
	
		public:
		
		
			__attribute__((noreturn))
			static void Raise ();
	
	
	};


	enum class ProtocolState {
	
		Handshaking,
		Play,
		Status,
		Login
	
	};
	
	
	enum class ProtocolDirection {
	
		Clientbound,
		Serverbound,
		Both
	
	};
	
	
	class Packet {
	
	
		protected:
		
		
			Packet (UInt32) noexcept;
			
			
			Packet (const Packet &) = default;
			Packet (Packet &&) = default;
			Packet & operator = (const Packet &) = default;
			Packet & operator = (Packet &&) = default;
	
	
		public:
		
		
			Packet () = delete;
		
		
			UInt32 ID;
			
			
			template <typename T>
			typename std::enable_if<
				std::is_base_of<Packet,T>::value,
				typename std::decay<T>::type
			>::type & Get () noexcept {
			
				return *reinterpret_cast<T *>(
					reinterpret_cast<void *>(
						this
					)
				);
			
			}
			
			
			template <typename T>
			const typename std::enable_if<
				std::is_base_of<Packet,T>::value,
				typename std::decay<T>::type
			>::type & Get () const noexcept {
			
				return *reinterpret_cast<const T *>(
					reinterpret_cast<const void *>(
						this
					)
				);
			
			}
	
	
	};


	/**
	 *	\cond
	 */


	namespace PacketImpl {
	
	
		template <typename T>
		class VarInt {
		
		
			private:
			
			
				T value;
				
				
			public:
			
			
				VarInt () = default;
				VarInt (T value) noexcept : value(std::move(value)) {	}
				VarInt & operator = (T value) noexcept {
				
					this->value=value;
					
					return *this;
				
				}
				operator T () const noexcept {
				
					return value;
				
				}
		
		
		};
		
		
	}
	
	
}


namespace std {


	template <typename T>
	struct numeric_limits<MCPP::PacketImpl::VarInt<T>> : public numeric_limits<T> {	};
	template <typename T>
	struct is_signed<MCPP::PacketImpl::VarInt<T>> : public is_signed<T> {	};
	template <typename T>
	struct is_unsigned<MCPP::PacketImpl::VarInt<T>> : public is_unsigned<T> {	};


}


namespace MCPP {


	namespace PacketImpl {
		
		
		template <Word i, typename... Args>
		class MimicLayout : public MimicLayout<i-1,Args...> {
		
			
			public:
			
			
				typename GetType<i,Args...>::Type Value;
			
		
		};
		
		
		template <typename... Args>
		class MimicLayout<0,Args...> {
		
		
			public:
			
			
				typename GetType<0,Args...>::Type Value;
		
		
		};
		
		
		template <Word i>
		class MimicLayout<i> {	};
		
		
		template <Word i, typename... Args>
		class GetOffset {
		
		
			private:
			
			
				typedef MimicLayout<i,Args...> type;
		
		
			public:
			
			
				#pragma GCC diagnostic push
				#pragma GCC diagnostic ignored "-Winvalid-offsetof"
				constexpr static Word Value=offsetof(type,Value);
				#pragma GCC diagnostic pop
		
		
		};
	
	
		template <typename... Args>
		class PacketType {
		
		
			private:
			
			
				typedef VarInt<UInt32> id_type;
		
		
			public:
			
			
				template <Word i>
				class Types {
				
				
					public:
					
					
						typedef typename GetType<i,Args...>::Type Type;
						
						
						constexpr static Word Offset=GetOffset<i+1,id_type,Args...>::Value;
				
				
				};
				
				
				constexpr static Word Count=sizeof...(Args);
				
				
				constexpr static Word Size=sizeof(MimicLayout<sizeof...(Args),id_type,Args...>);
		
		
		};
		
		
		template <typename prefix, typename type>
		class Array {
		
		
			public:
			
			
				Vector<type> Value;
		
		
		};
		
		
		template <ProtocolState, ProtocolDirection, UInt32>
		class PacketMap : public PacketType<> {	};
		
		
		constexpr ProtocolState HS=ProtocolState::Handshaking;
		constexpr ProtocolState PL=ProtocolState::Play;
		constexpr ProtocolState ST=ProtocolState::Status;
		constexpr ProtocolState LI=ProtocolState::Login;
		
		
		constexpr ProtocolDirection CB=ProtocolDirection::Clientbound;
		constexpr ProtocolDirection SB=ProtocolDirection::Serverbound;
		constexpr ProtocolDirection BO=ProtocolDirection::Both;
		
		
		typedef Int32 PLACEHOLDER;	//	Placeholder for unimplemented types
		
		
		//	HANDSHAKE
		
		//	SERVERBOUND
		template <> class PacketMap<HS,SB,0x00> : public PacketType<VarInt<UInt32>,String,UInt16,ProtocolState> {	};
		
		//	PLAY
		
		//	CLIENTBOUND
		template <> class PacketMap<PL,CB,0x00> : public PacketType<Int32> {	};
		template <> class PacketMap<PL,CB,0x01> : public PacketType<Int32,Byte,SByte,Byte,Byte> {	};
		template <> class PacketMap<PL,CB,0x02> : public PacketType<JSON::Value> {	};
		template <> class PacketMap<PL,CB,0x03> : public PacketType<Int64,Int64> {	};
		template <> class PacketMap<PL,CB,0x04> : public PacketType<Int32,Int16,PLACEHOLDER> {	};
		template <> class PacketMap<PL,CB,0x05> : public PacketType<Int32,Int32,Int32> {	};
		template <> class PacketMap<PL,CB,0x06> : public PacketType<Single,Int16,Single> {	};
		template <> class PacketMap<PL,CB,0x07> : public PacketType<Int32,Byte,Byte> {	};
		template <> class PacketMap<PL,CB,0x08> : public PacketType<Int32,Int32,Int32,Single,Single,bool> {	};
		template <> class PacketMap<PL,CB,0x09> : public PacketType<SByte> {	};
		template <> class PacketMap<PL,CB,0x0A> : public PacketType<Int32,Int32,Byte,Int32> {	};
		template <> class PacketMap<PL,CB,0x0B> : public PacketType<Int32,Byte> {	};
		template <> class PacketMap<PL,CB,0x0C> : public PacketType<VarInt<UInt32>,String,String,Int32,Int32,Int32,SByte,SByte,Int16,PLACEHOLDER> {	};
		template <> class PacketMap<PL,CB,0x0D> : public PacketType<Int32,Int32> {	};
		template <> class PacketMap<PL,CB,0x0E> : public PacketType<VarInt<UInt32>,SByte,Int32,Int32,Int32,SByte,SByte,PLACEHOLDER> {	};
		template <> class PacketMap<PL,CB,0x0F> : public PacketType<VarInt<UInt32>,Byte,Int32,Int32,Int32,SByte,SByte,SByte,Int16,Int16,Int16,PLACEHOLDER> {	};
		template <> class PacketMap<PL,CB,0x10> : public PacketType<VarInt<UInt32>,String,Int32,Int32,Int32,Int32> {	};
		template <> class PacketMap<PL,CB,0x11> : public PacketType<VarInt<UInt32>,Int32,Int32,Int32,Int16> {	};
		template <> class PacketMap<PL,CB,0x12> : public PacketType<VarInt<UInt32>,Int16,Int16,Int16> {	};
		template <> class PacketMap<PL,CB,0x13> : public PacketType<VarInt<UInt32>,Array<SByte,Int32>> {	};
		
		
		constexpr UInt32 LargestID=0x40;
		
		
		constexpr ProtocolState Next (ProtocolState ps) noexcept {
		
			return (ps==HS) ? PL : ((ps==PL) ? ST : LI);
		
		}
		
		
		constexpr ProtocolDirection Next (ProtocolDirection pd) noexcept {
		
			return (pd==CB) ? SB : ((pd==SB) ? BO : CB);
		
		}
		
		
		template <typename T>
		constexpr T Max (T a, T b) noexcept {
		
			return (a>b) ? a : b;
		
		}
		
		
		template <ProtocolState ps, ProtocolDirection pd, UInt32 id>
		class LargestImpl {
		
		
			public:
			
			
				constexpr static Word Value=Max(
					PacketMap<ps,pd,id>::Size,
					LargestImpl<ps,pd,id+1>::Value
				);
		
		
		};
		
		
		template <ProtocolState ps, ProtocolDirection pd>
		class LargestImpl<ps,pd,LargestID> {
		
		
			public:
			
			
				constexpr static Word Value=Max(
					PacketMap<ps,pd,LargestID>::Size,
					LargestImpl<
						(pd==BO) ? Next(ps) : ps,
						Next(pd),
						0
					>::Value
				);
		
		
		};
		
		
		template <>
		class LargestImpl<LI,BO,LargestID> {
		
		
			public:
			
			
				constexpr static Word Value=PacketMap<LI,BO,LargestID>::Size;
		
		
		};
		
		
		constexpr Word Largest=LargestImpl<HS,CB,0>::Value;
		
		
		class PacketContainer {
		
		
			public:
			
			
				typedef void (*destroy_type) (void *);
				typedef void (*from_bytes_type) (const Byte * &, const Byte *, void *);
		
		
			protected:
			
			
				destroy_type destroy;
				from_bytes_type from_bytes;
				bool engaged;
				alignas(Packet) Byte storage [Largest];
				
				
				inline void destroy_impl () noexcept;
		
		
			public:
			
			
				PacketContainer () noexcept;
			
			
				PacketContainer (const PacketContainer &) = delete;
				PacketContainer (PacketContainer &&) = delete;
				PacketContainer & operator = (const PacketContainer &) = delete;
				PacketContainer & operator = (PacketContainer &&) = delete;
			
			
				~PacketContainer () noexcept;
				
				
				void FromBytes (const Byte * &, const Byte *);
				Packet & Get () noexcept;
				const Packet & Get () const noexcept;
				
				
				void Imbue (destroy_type, from_bytes_type) noexcept;
		
		
		};
		
		
		template <typename T>
		class Serializer {
		
		
			public:
			
			
				constexpr static Word Size (const T &) noexcept {
				
					return sizeof(T);
				
				}
				
				
				static void FromBytes (const Byte * & begin, const Byte * end, void * ptr) noexcept {
				
					//	Make sure there's enough space
					//	to extract an object of this
					//	type from the buffer
					if ((end-begin)<sizeof(T)) InsufficientBytes::Raise();
					
					//	Copy bytes from the buffer
					//	to the object pointer
					std::memcpy(ptr,begin,sizeof(T));
					
					//	Advance the begin pointer
					begin+=sizeof(T);
					
					//	If the system is not big endian,
					//	reverse bytes so in memory
					//	representation is of the proper
					//	endianness
					if (!Endianness::IsBigEndian<T>()) {
					
						Byte * byte_ptr=reinterpret_cast<Byte *>(ptr);
						
						for (Word i=0;i<(sizeof(T)/2);++i) std::swap(
							byte_ptr[i],
							byte_ptr[sizeof(T)-i-1]
						);
					
					}
				
				}
				
				
				static void ToBytes (Vector<Byte> & buffer, const T & obj) {
				
					//	Resize buffer as/if necessary
					while ((buffer.Capacity()-buffer.Count())<sizeof(T)) buffer.SetCapacity();
					
					auto end=buffer.end();
					
					//	Copy bytes
					std::memcpy(end,&obj,sizeof(T));
					
					//	If system is not big endian, we
					//	must reverse bytes in the buffer
					if (!Endianness::IsBigEndian<T>()) for (Word i=0;i<(sizeof(T)/2);++i) std::swap(
						end[i],
						end[sizeof(T)-i-1]
					);
					
					//	Set buffer count
					buffer.SetCount(buffer.Count()+sizeof(T));
				
				}
		
		
		};
		
		
		template <typename T>
		T Deserialize (const Byte * & begin, const Byte * end) {
		
			alignas(T) Byte buffer [sizeof(T)];
			
			Serializer<T>::FromBytes(begin,end,buffer);
			
			Nullable<T> retr;
			T * ptr=reinterpret_cast<T *>(
				reinterpret_cast<void *>(
					buffer
				)
			);
			
			try {
			
				retr.Construct(std::move(*ptr));
				
			} catch (...) {
			
				ptr->~T();
				
				throw;
			
			}
			
			ptr->~T();
			
			return std::move(*retr);
		
		}
		
		
		template <typename T>
		class Serializer<VarInt<T>> {
		
		
			private:
			
			
				typedef typename std::make_unsigned<T>::type type;
				
				
				constexpr static bool is_signed=std::is_signed<T>::value;
				
				
				constexpr static Word bits=sizeof(T)*BitsPerByte();
				
				
				constexpr static Word max_bytes=((bits%7)==0) ? (bits/7) : ((bits/7)+1);
				
				
				constexpr static Byte get_final_mask_impl (Word target, Word i) noexcept {
				
					return (i==target) ? 0 : ((static_cast<Byte>(1)<<i)|get_final_mask_impl(target,i+1));
				
				}
				
				
				constexpr static Byte get_final_mask () noexcept {
				
					return (~get_final_mask_impl(bits%7,0))&127;
				
				}
		
		
			public:
			
			
				static Word Size (VarInt<T> obj) noexcept {
				
					union {
						type value;
						T s_value;
					};
					
					s_value=obj;
					
					UInt32 mask=127;
					for (Word i=1;i<(max_bytes-1);++i) {
					
						if ((value&~mask)==0) return i;
						
						mask<<=7;
						mask|=127;
					
					}
					
					return max_bytes;
				
				}
				
				
				static void FromBytes (const Byte * & begin, const Byte * end, void * ptr) {
				
					//	Build a value from the bytes in
					//	the buffer -- start at zero
					type value=0;
					
					//	When this is set to true, we've
					//	reached the end of the VarInt,
					//	i.e. the msb was not set
					bool complete=false;
					for (
						Word i=1;
						!((begin==end) || complete);
						++i
					) {
					
						Byte b=*(begin++);
						
						//	If msb is set, we're done
						//	after considering this byte
						if ((b&128)==0) complete=true;
						//	Otherwise we ignore msb
						else b&=~static_cast<Byte>(128);
						
						//	On the last byte we do some
						//	special processing to make sure
						//	that the integer we're decoding
						//	doesn't overflow
						if (
							(i==max_bytes) &&
							//	We need to make sure bits
							//	that would cause an overflow
							//	are not set
							((i&get_final_mask())!=0)
						) BadFormat::Raise();
						
						//	Add the value of this byte to
						//	the value we're building
						value|=static_cast<type>(b)<<(7*(i-1));
						
						//	If this is the last possible
						//	byte, and we've not reached
						//	the end of the VarInt, there's
						//	a problem with the format of
						//	the data
						if (
							!complete &&
							(i==max_bytes)
						) BadFormat::Raise();
					
					}
					
					//	If the VarInt is not complete,
					//	we didn't finish
					if (!complete) InsufficientBytes::Raise();
					
					T final;
					
					//	If value is signed, use zig zag
					//	encoding
					if (is_signed) {
					
						//	Zero is always zero
						if (value==0) {
						
							final=static_cast<T>(value);
							
						} else {
						
							final=static_cast<T>(value/2);
						
							if ((value%2)==1) {
							
								//	Value is negative
								
								final=(
									(value==std::numeric_limits<type>::max())
										?	std::numeric_limits<T>::min()
										:	((final*-1)-1)
								);
							
							}
							
						}
					
					//	If value is unsigned, just take
					//	value as it is
					} else {
					
						final=static_cast<T>(value);
					
					}
					
					new (ptr) VarInt<T> (final);
				
				}
				
				
				static void ToBytes (Vector<Byte> & buffer, VarInt<T> obj) {
				
					union {
						type value;
						T s_value;
					};
					
					s_value=obj;
					
					//	Zig zag encode if value is
					//	signed
					if (is_signed) {
					
						//	Zero is left alone
					
						if (s_value<0) value=(
							(s_value==std::numeric_limits<T>::min())
								?	std::numeric_limits<type>::max()
								:	((static_cast<type>(s_value*-1)*2)-1)
						);
						else if (s_value>0) value*=2;
						
					}
					
					do {
					
						//	Get bottom seven bits
						Byte b=static_cast<Byte>(value&127);
						
						//	Discard bottom seven bits
						value>>=7;
						
						//	Set msb if necessary
						if (value!=0) b|=128;
						
						//	Add
						buffer.Add(b);
					
					} while (value!=0);
				
				}
		
		
		};
		
		
		template <>
		class Serializer<String> {
		
		
			private:
			
			
				typedef UInt32 size_type;
				typedef VarInt<size_type> var_int_type;
		
		
			public:
			
			
				static Word Size (const String & obj) {
				
					SafeWord safe(obj.Size());
					
					//	Maximum number of UTF-8 code units per code point
					safe*=4;
					
					//	Size of the VarInt which describes the maximum
					//	number of code units (i.e. bytes) in the string
					var_int_type var_int=size_type(safe);
					
					//	Add VarInt byte count to code unit (i.e. byte)
					//	count
					safe+=SafeWord(Serializer<decltype(var_int)>::Size(var_int));
					
					return Word(safe);
				
				}
				
				
				static void FromBytes (const Byte * & begin, const Byte * end, void * ptr) {
				
					//	Get length of string in bytes
					var_int_type len=Deserialize<decltype(len)>(begin,end);
					
					//	Check to make sure there's enough
					//	bytes for this string to actually
					//	be in the buffer
					if ((end-begin)<size_type(len)) InsufficientBytes::Raise();
					
					auto start=begin;
					begin+=size_type(len);
					
					//	Decode
					new (ptr) String (UTF8().Decode(start,begin));
				
				}
				
				
				static void ToBytes (Vector<Byte> & buffer, const String & obj) {
				
					//	Get UTF-8 encoding of string
					auto encoded=UTF8().Encode(obj);
					
					//	Place the VarInt encoding of the
					//	string in the buffer
					Serializer<var_int_type>::ToBytes(
						buffer,
						size_type(SafeWord(encoded.Count()))
					);
					
					//	Make enough space in the buffer
					//	for the string
					while ((buffer.Capacity()-buffer.Count())<encoded.Count()) buffer.SetCapacity();
					
					//	Copy
					std::memcpy(
						buffer.end(),
						encoded.begin(),
						encoded.Count()
					);
					
					buffer.SetCount(buffer.Count()+encoded.Count());
				
				}
		
		
		};
		
		
		template <>
		class Serializer<JSON::Value> {
		
		
			private:
			
			
				//	Maximum recursion the JSON parser
				//	will be willing to go through before
				//	bailing out and throwing when parsing
				//	incoming JSON
				constexpr static Word max_recursion=10;
		
		
			public:
			
			
				constexpr static Word Size (const JSON::Value &) {
				
					//	This is unknowable without actually
					//	serializing
					return 0;
				
				}
				
				
				static void FromBytes (const Byte * & begin, const Byte * end, void * ptr) {
				
					//	Get the string from which we'll
					//	extract JSON
					auto str=Deserialize<String>(begin,end);
					
					//	Parse JSON
					new (ptr) JSON::Value (JSON::Parse(str,max_recursion));
				
				}
				
				
				static void ToBytes (Vector<Byte> & buffer, const JSON::Value & obj) {
				
					Serializer<String>::ToBytes(
						buffer,
						JSON::Serialize(obj)
					);
				
				}
		
		
		};
		
		
		template <>
		class Serializer<ProtocolState> {
		
		
			public:
			
			
				constexpr static Word Size (ProtocolState) {
				
					//	Only ever 1 byte
					return 0;
				
				}
				
				
				static void FromBytes (const Byte * & begin, const Byte * end, void * ptr) {
				
					//	Get the raw byte
					auto b=Deserialize<Byte>(begin,end);
					
					//	Only valid values are 1 or 2
					if (!((b==1) || (b==2))) BadFormat::Raise();
					
					//	Create
					new (ptr) ProtocolState ((b==1) ? ProtocolState::Status : ProtocolState::Login);
				
				}
				
				
				static void ToBytes (Vector<Byte> & buffer, ProtocolState state) {
				
					Byte b;
					switch (state) {
					
						case ProtocolState::Status:
							b=1;
							break;
						case ProtocolState::Login:
							b=2;
							break;
						default:
							BadFormat::Raise();
					
					}
					
					Serializer<Byte>::ToBytes(buffer,b);
				
				}
		
		
		};
		
		
		template <typename T>
		class GetIntegerType {
		
		
			public:
			
			
				typedef T Type;
		
		
		};
		
		
		template <typename T>
		class GetIntegerType<VarInt<T>> {
		
		
			public:
			
			
				typedef T Type;
		
		
		};

		
		template <typename prefix, typename inner>
		class Serializer<Array<prefix,inner>> {
		
		
			private:
			
			
				typedef Array<prefix,inner> type;
				typedef typename GetIntegerType<prefix>::Type int_type;
		
		
			public:
			
			
				static Word Size (const type & obj) {
				
					SafeWord size=Serializer<prefix>::Size(obj.Value.Count());
					
					for (const auto & i : obj.Value) size+=SafeWord(Serializer<inner>::Size(i));
					
					return Word(size);
				
				}
				
				
				static void FromBytes (const Byte * & begin, const Byte * end, void * ptr) {
				
					//	Get array count
					auto count=Deserialize<prefix>(begin,end);
					
					//	Check to make sure array
					//	count is not negative
					if (
						std::is_signed<prefix>::value &&
						(count<0)
					) BadFormat::Raise();
					
					Vector<inner> vec;
					
					//	Loop and get each element in
					//	the array
					for (Word i=0;i<count;++i) vec.Add(Deserialize<inner>(begin,end));
					
					new (ptr) Vector<inner> (std::move(vec));
				
				}
				
				
				static void ToBytes (Vector<Byte> & buffer, const type & obj) {
				
					//	Serialize count
					Serializer<prefix>::ToBytes(
						buffer,
						int_type(SafeWord(obj.Value.Count()))
					);
					
					//	Serialize each object
					for (const auto & i : obj.Value) Serializer<inner>::ToBytes(buffer,i);
				
				}
		
		
		};
		
		
		template <typename... Args>
		class Serializer<Tuple<Args...>> {
		
		
			private:
			
			
				typedef Tuple<Args...> type;
				
				
				template <Word i>
				constexpr static typename std::enable_if<
					i>=sizeof...(Args),
					Word
				>::type size_impl (const type &) noexcept {
				
					return 0;
				
				}
			
			
				template <Word i>
				static typename std::enable_if<
					i<sizeof...(Args),
					Word
				>::type size_impl (const type & obj) {
				
					const auto & curr=obj.template Item<i>();
					
					return Word(
						SafeWord(Serializer<typename std::decay<decltype(curr)>::type>::Size(curr))+
						SafeWord(size_impl<i+1>(obj))
					);
				
				}
				
				
				template <Word i>
				static typename std::enable_if<
					i>=sizeof...(Args)
				>::type serialize_impl (Vector<Byte> &, const type &) noexcept {	}
				
				
				template <Word i>
				static typename std::enable_if<
					i<sizeof...(Args)
				>::type serialize_impl (Vector<Byte> & buffer, const type & obj) {
				
					const auto & curr=obj.template Item<i>();
					
					Serializer<typename std::decay<decltype(curr)>::type>::ToBytes(buffer,curr);
					
					serialize_impl<i+1>(buffer,obj);
				
				}
				
				
				template <Word i>
				static typename std::enable_if<
					i>=sizeof...(Args)
				>::type deserialize_impl (const Byte * &, const Byte *, type &) noexcept {	}
				
				
				template <Word i>
				static typename std::enable_if<
					i<sizeof...(Args)
				>::type deserialize_impl (const Byte * & begin, const Byte * end, type & obj) {
				
					auto & curr=obj.template Item<i>();
					
					typedef typename std::decay<decltype(curr)>::type curr_type;
					
					Serializer<curr_type>::FromBytes(begin,end,&curr);
					
					try {
					
						deserialize_impl<i+1>(begin,end,obj);
					
					} catch (...) {
					
						curr.~curr_type();
						
						throw;
					
					}
				
				}
		
			
			public:
			
			
				static Word Size (const type & obj) {
				
					return size_impl<0>(obj);
				
				}
				
				
				static void FromBytes (const Byte * & begin, const Byte * end, void * ptr) {
				
					deserialize_impl<0>(
						begin,
						end,
						*reinterpret_cast<type *>(ptr)
					);
				
				}
				
				
				static void ToBytes (Vector<Byte> & buffer, const type & obj) {
				
					serialize_impl<0>(buffer,obj);
				
				}
			
		
		};
	
	
	}
	
	
	/**
	 *	\endcond
	 */
	 
	 
	class PacketParser {
	
	
		private:
		
		
			PacketImpl::PacketContainer container;
			bool in_progress;
			Word waiting_for;
			
			
		public:
		
		
			PacketParser () noexcept;
		
		
			bool FromBytes (Vector<Byte> & buffer, ProtocolState state, ProtocolDirection direction);
			
			
			Packet & Get () noexcept;
			const Packet & Get () const noexcept;
	
	
	};
	
	
	namespace Packets {
	
	
		/**
		 *	\cond
		 */
	
	
		class HSPacket {
		
		
			public:
			
			
				constexpr static ProtocolState State=ProtocolState::Handshaking;
		
		
		};
		
		
		class PLPacket {
		
		
			public:
			
			
				constexpr static ProtocolState State=ProtocolState::Play;
		
		
		};
		
		
		class STPacket {
		
		
			public:
			
			
				constexpr static ProtocolState State=ProtocolState::Status;
		
		
		};
		
		
		class LIPacket {
		
		
			public:
			
			
				constexpr static ProtocolState State=ProtocolState::Login;
		
		
		};
		
		
		class CBPacket {
		
		
			public:
			
			
				constexpr static ProtocolDirection Direction=ProtocolDirection::Clientbound;
		
		
		};
		
		
		class SBPacket {
		
		
			public:
			
			
				constexpr static ProtocolDirection Direction=ProtocolDirection::Serverbound;
		
		
		};
		
		
		class BOPacket {
		
		
			public:
			
			
				constexpr static ProtocolDirection Direction=ProtocolDirection::Both;
		
		
		};
		
		
		template <UInt32 id>
		class IDPacket : public Packet {
		
		
			public:
			
			
				constexpr static UInt32 PacketID=id;
				
				
				IDPacket () noexcept : Packet(id) {	}
		
		
		};
		
		
		/**
		 *	\endcond
		 */
	
	
		namespace Handshaking {
		
		
			namespace Serverbound {
			
			
				/**
				 *	\cond
				 */
				 
				 
				class Base : public HSPacket, public SBPacket {	};
				
				
				/**
				 *	\endcond
				 */
			
			
				class Handshake : public Base, public IDPacket<0x00> {
				
				
					public:
					
					
						UInt32 ProtocolVersion;
						String ServerAddress;
						UInt16 ServerPort;
						ProtocolState CurrentState;
				
				
				};
			
			
			}
		
		
		}
		
		
		namespace Play {
		
		
			namespace Clientbound {
			
			
				/**
				 *	\cond
				 */
				 
				 
				class Base : public PLPacket, public CBPacket {	};
				
				
				/**
				 *	\endcond
				 */
				 
				 
				class KeepAlive : public Base, public IDPacket<0x00> {
				
				
					public:
					
					
						Int32 KeepAliveID;
				
				
				};
				
				
				class JoinGame : public Base, public IDPacket<0x01> {
				
				
					public:
					
					
						Int32 EntityID;
						Byte GameMode;
						SByte Dimension;
						Byte Difficulty;
						Byte MaxPlayers;
				
				
				};
				
				
				class ChatMessage : public Base, public IDPacket<0x02> {
				
				
					public:
					
					
						JSON::Value Value;
				
				
				};
				
				
				class TimeUpdate : public Base, public IDPacket<0x03> {
				
				
					public:
					
					
						Int64 Age;
						Int64 Time;
				
				
				};
				
				
				class EntityEquipment : public Base, public IDPacket<0x04> {
				
				
					public:
					
					
						Int32 EntityID;
				
				
				};
				
				
				class SpawnPosition : public Base, public IDPacket<0x05> {
				
				
					public:
					
					
						Int32 X;
						Int32 Y;
						Int32 Z;
				
				
				};
				
				
				class UpdateHealth : public Base, public IDPacket<0x06> {
				
				
					public:
					
					
						Single Health;
						Int16 Food;
						Single Saturation;
				
				
				};
			
			
			}
		
		
		}
	
	
	}
	
	
	/**
	 *	\cond
	 */
	
	
	namespace PacketImpl {
	
	
		template <Word i, typename T>
		constexpr typename std::enable_if<
			i>=T::Count,
			Word
		>::type SizeImpl (const void *) noexcept {
		
			return 0;
		
		}
		
		
		template <Word i, typename T>
		typename std::enable_if<
			i<T::Count,
			Word
		>::type SizeImpl (const void * ptr) {
		
			typedef typename T::template Types<i> types;
			typedef typename types::Type type;
		
			return Word(
				SafeWord(
					Serializer<type>::Size(
						*reinterpret_cast<const type *>(
							reinterpret_cast<const void *>(
								reinterpret_cast<const Byte *>(ptr)+types::Offset
							)
						)
					)
				)+
				SafeWord(SizeImpl<i+1,T>(ptr))
			);
		
		}
		
		
		template <Word i, typename T>
		typename std::enable_if<
			i>=T::Count
		>::type SerializeImpl (const Vector<Byte> &, const void *) noexcept {	}
		
		
		template <Word i, typename T>
		typename std::enable_if<
			i<T::Count
		>::type SerializeImpl (Vector<Byte> & buffer, const void * ptr) {
		
			typedef typename T::template Types<i> types;
			typedef typename types::Type type;
			
			Serializer<type>::ToBytes(
				buffer,
				*reinterpret_cast<const type *>(
					reinterpret_cast<const void *>(
						reinterpret_cast<const Byte *>(ptr)+types::Offset
					)
				)
			);
			
			SerializeImpl<i+1,T>(buffer,ptr);
		
		}
	
	
	}
	
	
	/**
	 *	\endcond
	 */
	
	
	template <typename T>
	typename std::enable_if<
		PacketImpl::PacketMap<T::State,T::Direction,T::PacketID>::Count!=0,
		Vector<Byte>
	>::type Serialize (const T & packet) {
	
		using namespace PacketImpl;
	
		typedef PacketMap<T::State,T::Direction,T::PacketID> type;
		typedef Serializer<VarInt<UInt32>> serializer;
		
		const void * ptr=&packet;
		const VarInt<UInt32> & id=*reinterpret_cast<const VarInt<UInt32> *>(ptr);
		
		Vector<Byte> buffer(
			Word(
				SafeWord(SizeImpl<0,type>(ptr))+
				SafeWord(serializer::Size(id))
			)
		);
		
		serializer::ToBytes(buffer,id);
		
		SerializeImpl<0,type>(buffer,ptr);
		
		UInt32 len=UInt32(
			SafeWord(
				buffer.Count()
			)
		);
		
		Word final_len=Word(
			SafeWord(serializer::Size(len))+
			SafeWord(buffer.Count())
		);
		
		Vector<Byte> retr(final_len);
		
		serializer::ToBytes(retr,len);
		
		Word len_count=retr.Count();
		
		std::memcpy(
			retr.end(),
			buffer.begin(),
			buffer.Count()
		);
		
		retr.SetCount(len_count+buffer.Count());
		
		return retr;
	
	}


}
