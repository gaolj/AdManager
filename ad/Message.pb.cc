// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: Message.proto

#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION
#include "Message.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/port.h>
#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)
class MessageDefaultTypeInternal : public ::google::protobuf::internal::ExplicitlyConstructed<Message> {
} _Message_default_instance_;

namespace protobuf_Message_2eproto {


namespace {

::google::protobuf::Metadata file_level_metadata[1];

}  // namespace

const ::google::protobuf::uint32 TableStruct::offsets[] = {
  ~0u,  // no _has_bits_
  GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Message, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Message, id_),
  GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Message, method_),
  GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Message, content_),
  GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Message, returncode_),
  GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Message, returnmsg_),
};

static const ::google::protobuf::internal::MigrationSchema schemas[] = {
  { 0, -1, sizeof(Message)},
};

static ::google::protobuf::Message const * const file_default_instances[] = {
  reinterpret_cast<const ::google::protobuf::Message*>(&_Message_default_instance_),
};

namespace {

void protobuf_AssignDescriptors() {
  AddDescriptors();
  ::google::protobuf::MessageFactory* factory = NULL;
  AssignDescriptors(
      "Message.proto", schemas, file_default_instances, TableStruct::offsets, factory,
      file_level_metadata, NULL, NULL);
}

void protobuf_AssignDescriptorsOnce() {
  static GOOGLE_PROTOBUF_DECLARE_ONCE(once);
  ::google::protobuf::GoogleOnceInit(&once, &protobuf_AssignDescriptors);
}

void protobuf_RegisterTypes(const ::std::string&) GOOGLE_ATTRIBUTE_COLD;
void protobuf_RegisterTypes(const ::std::string&) {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::internal::RegisterAllTypes(file_level_metadata, 1);
}

}  // namespace

void TableStruct::Shutdown() {
  _Message_default_instance_.Shutdown();
  delete file_level_metadata[0].reflection;
}

void TableStruct::InitDefaultsImpl() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::google::protobuf::internal::InitProtobufDefaults();
  _Message_default_instance_.DefaultConstruct();
}

void InitDefaults() {
  static GOOGLE_PROTOBUF_DECLARE_ONCE(once);
  ::google::protobuf::GoogleOnceInit(&once, &TableStruct::InitDefaultsImpl);
}
void AddDescriptorsImpl() {
  InitDefaults();
  static const char descriptor[] = {
      "\n\rMessage.proto\"]\n\007Message\022\n\n\002id\030\001 \001(\003\022\016"
      "\n\006method\030\002 \001(\t\022\017\n\007content\030\003 \001(\014\022\022\n\nretur"
      "nCode\030\004 \001(\005\022\021\n\treturnMsg\030\005 \001(\tb\006proto3"
  };
  ::google::protobuf::DescriptorPool::InternalAddGeneratedFile(
      descriptor, 118);
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedFile(
    "Message.proto", &protobuf_RegisterTypes);
  ::google::protobuf::internal::OnShutdown(&TableStruct::Shutdown);
}

void AddDescriptors() {
  static GOOGLE_PROTOBUF_DECLARE_ONCE(once);
  ::google::protobuf::GoogleOnceInit(&once, &AddDescriptorsImpl);
}
// Force AddDescriptors() to be called at static initialization time.
struct StaticDescriptorInitializer {
  StaticDescriptorInitializer() {
    AddDescriptors();
  }
} static_descriptor_initializer;

}  // namespace protobuf_Message_2eproto


// ===================================================================

#if !defined(_MSC_VER) || _MSC_VER >= 1900
const int Message::kIdFieldNumber;
const int Message::kMethodFieldNumber;
const int Message::kContentFieldNumber;
const int Message::kReturnCodeFieldNumber;
const int Message::kReturnMsgFieldNumber;
#endif  // !defined(_MSC_VER) || _MSC_VER >= 1900

Message::Message()
  : ::google::protobuf::Message(), _internal_metadata_(NULL) {
  if (GOOGLE_PREDICT_TRUE(this != internal_default_instance())) {
    protobuf_Message_2eproto::InitDefaults();
  }
  SharedCtor();
  // @@protoc_insertion_point(constructor:Message)
}
Message::Message(const Message& from)
  : ::google::protobuf::Message(),
      _internal_metadata_(NULL),
      _cached_size_(0) {
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  method_.UnsafeSetDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  if (from.method().size() > 0) {
    method_.AssignWithDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), from.method_);
  }
  content_.UnsafeSetDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  if (from.content().size() > 0) {
    content_.AssignWithDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), from.content_);
  }
  returnmsg_.UnsafeSetDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  if (from.returnmsg().size() > 0) {
    returnmsg_.AssignWithDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), from.returnmsg_);
  }
  ::memcpy(&id_, &from.id_,
    reinterpret_cast<char*>(&returncode_) -
    reinterpret_cast<char*>(&id_) + sizeof(returncode_));
  // @@protoc_insertion_point(copy_constructor:Message)
}

void Message::SharedCtor() {
  method_.UnsafeSetDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  content_.UnsafeSetDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  returnmsg_.UnsafeSetDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  ::memset(&id_, 0, reinterpret_cast<char*>(&returncode_) -
    reinterpret_cast<char*>(&id_) + sizeof(returncode_));
  _cached_size_ = 0;
}

Message::~Message() {
  // @@protoc_insertion_point(destructor:Message)
  SharedDtor();
}

void Message::SharedDtor() {
  method_.DestroyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  content_.DestroyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  returnmsg_.DestroyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}

void Message::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* Message::descriptor() {
  protobuf_Message_2eproto::protobuf_AssignDescriptorsOnce();
  return protobuf_Message_2eproto::file_level_metadata[0].descriptor;
}

const Message& Message::default_instance() {
  protobuf_Message_2eproto::InitDefaults();
  return *internal_default_instance();
}

Message* Message::New(::google::protobuf::Arena* arena) const {
  Message* n = new Message;
  if (arena != NULL) {
    arena->Own(n);
  }
  return n;
}

void Message::Clear() {
// @@protoc_insertion_point(message_clear_start:Message)
  method_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  content_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  returnmsg_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  ::memset(&id_, 0, reinterpret_cast<char*>(&returncode_) -
    reinterpret_cast<char*>(&id_) + sizeof(returncode_));
}

bool Message::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!GOOGLE_PREDICT_TRUE(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  // @@protoc_insertion_point(parse_start:Message)
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoffNoLastTag(127u);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // int64 id = 1;
      case 1: {
        if (tag == 8u) {

          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int64, ::google::protobuf::internal::WireFormatLite::TYPE_INT64>(
                 input, &id_)));
        } else {
          goto handle_unusual;
        }
        break;
      }

      // string method = 2;
      case 2: {
        if (tag == 18u) {
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_method()));
          DO_(::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
            this->method().data(), this->method().length(),
            ::google::protobuf::internal::WireFormatLite::PARSE,
            "Message.method"));
        } else {
          goto handle_unusual;
        }
        break;
      }

      // bytes content = 3;
      case 3: {
        if (tag == 26u) {
          DO_(::google::protobuf::internal::WireFormatLite::ReadBytes(
                input, this->mutable_content()));
        } else {
          goto handle_unusual;
        }
        break;
      }

      // int32 returnCode = 4;
      case 4: {
        if (tag == 32u) {

          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &returncode_)));
        } else {
          goto handle_unusual;
        }
        break;
      }

      // string returnMsg = 5;
      case 5: {
        if (tag == 42u) {
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_returnmsg()));
          DO_(::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
            this->returnmsg().data(), this->returnmsg().length(),
            ::google::protobuf::internal::WireFormatLite::PARSE,
            "Message.returnMsg"));
        } else {
          goto handle_unusual;
        }
        break;
      }

      default: {
      handle_unusual:
        if (tag == 0 ||
            ::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          goto success;
        }
        DO_(::google::protobuf::internal::WireFormatLite::SkipField(input, tag));
        break;
      }
    }
  }
success:
  // @@protoc_insertion_point(parse_success:Message)
  return true;
failure:
  // @@protoc_insertion_point(parse_failure:Message)
  return false;
#undef DO_
}

void Message::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // @@protoc_insertion_point(serialize_start:Message)
  // int64 id = 1;
  if (this->id() != 0) {
    ::google::protobuf::internal::WireFormatLite::WriteInt64(1, this->id(), output);
  }

  // string method = 2;
  if (this->method().size() > 0) {
    ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
      this->method().data(), this->method().length(),
      ::google::protobuf::internal::WireFormatLite::SERIALIZE,
      "Message.method");
    ::google::protobuf::internal::WireFormatLite::WriteStringMaybeAliased(
      2, this->method(), output);
  }

  // bytes content = 3;
  if (this->content().size() > 0) {
    ::google::protobuf::internal::WireFormatLite::WriteBytesMaybeAliased(
      3, this->content(), output);
  }

  // int32 returnCode = 4;
  if (this->returncode() != 0) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(4, this->returncode(), output);
  }

  // string returnMsg = 5;
  if (this->returnmsg().size() > 0) {
    ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
      this->returnmsg().data(), this->returnmsg().length(),
      ::google::protobuf::internal::WireFormatLite::SERIALIZE,
      "Message.returnMsg");
    ::google::protobuf::internal::WireFormatLite::WriteStringMaybeAliased(
      5, this->returnmsg(), output);
  }

  // @@protoc_insertion_point(serialize_end:Message)
}

::google::protobuf::uint8* Message::InternalSerializeWithCachedSizesToArray(
    bool deterministic, ::google::protobuf::uint8* target) const {
  (void)deterministic;  // Unused
  // @@protoc_insertion_point(serialize_to_array_start:Message)
  // int64 id = 1;
  if (this->id() != 0) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt64ToArray(1, this->id(), target);
  }

  // string method = 2;
  if (this->method().size() > 0) {
    ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
      this->method().data(), this->method().length(),
      ::google::protobuf::internal::WireFormatLite::SERIALIZE,
      "Message.method");
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        2, this->method(), target);
  }

  // bytes content = 3;
  if (this->content().size() > 0) {
    target =
      ::google::protobuf::internal::WireFormatLite::WriteBytesToArray(
        3, this->content(), target);
  }

  // int32 returnCode = 4;
  if (this->returncode() != 0) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt32ToArray(4, this->returncode(), target);
  }

  // string returnMsg = 5;
  if (this->returnmsg().size() > 0) {
    ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
      this->returnmsg().data(), this->returnmsg().length(),
      ::google::protobuf::internal::WireFormatLite::SERIALIZE,
      "Message.returnMsg");
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        5, this->returnmsg(), target);
  }

  // @@protoc_insertion_point(serialize_to_array_end:Message)
  return target;
}

size_t Message::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:Message)
  size_t total_size = 0;

  // string method = 2;
  if (this->method().size() > 0) {
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::StringSize(
        this->method());
  }

  // bytes content = 3;
  if (this->content().size() > 0) {
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::BytesSize(
        this->content());
  }

  // string returnMsg = 5;
  if (this->returnmsg().size() > 0) {
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::StringSize(
        this->returnmsg());
  }

  // int64 id = 1;
  if (this->id() != 0) {
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::Int64Size(
        this->id());
  }

  // int32 returnCode = 4;
  if (this->returncode() != 0) {
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::Int32Size(
        this->returncode());
  }

  int cached_size = ::google::protobuf::internal::ToCachedSize(total_size);
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = cached_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void Message::MergeFrom(const ::google::protobuf::Message& from) {
// @@protoc_insertion_point(generalized_merge_from_start:Message)
  GOOGLE_DCHECK_NE(&from, this);
  const Message* source =
      ::google::protobuf::internal::DynamicCastToGenerated<const Message>(
          &from);
  if (source == NULL) {
  // @@protoc_insertion_point(generalized_merge_from_cast_fail:Message)
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
  // @@protoc_insertion_point(generalized_merge_from_cast_success:Message)
    MergeFrom(*source);
  }
}

void Message::MergeFrom(const Message& from) {
// @@protoc_insertion_point(class_specific_merge_from_start:Message)
  GOOGLE_DCHECK_NE(&from, this);
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  if (from.method().size() > 0) {

    method_.AssignWithDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), from.method_);
  }
  if (from.content().size() > 0) {

    content_.AssignWithDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), from.content_);
  }
  if (from.returnmsg().size() > 0) {

    returnmsg_.AssignWithDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), from.returnmsg_);
  }
  if (from.id() != 0) {
    set_id(from.id());
  }
  if (from.returncode() != 0) {
    set_returncode(from.returncode());
  }
}

void Message::CopyFrom(const ::google::protobuf::Message& from) {
// @@protoc_insertion_point(generalized_copy_from_start:Message)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void Message::CopyFrom(const Message& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:Message)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool Message::IsInitialized() const {
  return true;
}

void Message::Swap(Message* other) {
  if (other == this) return;
  InternalSwap(other);
}
void Message::InternalSwap(Message* other) {
  method_.Swap(&other->method_);
  content_.Swap(&other->content_);
  returnmsg_.Swap(&other->returnmsg_);
  std::swap(id_, other->id_);
  std::swap(returncode_, other->returncode_);
  std::swap(_cached_size_, other->_cached_size_);
}

::google::protobuf::Metadata Message::GetMetadata() const {
  protobuf_Message_2eproto::protobuf_AssignDescriptorsOnce();
  return protobuf_Message_2eproto::file_level_metadata[0];
}

#if PROTOBUF_INLINE_NOT_IN_HEADERS
// Message

// int64 id = 1;
void Message::clear_id() {
  id_ = GOOGLE_LONGLONG(0);
}
::google::protobuf::int64 Message::id() const {
  // @@protoc_insertion_point(field_get:Message.id)
  return id_;
}
void Message::set_id(::google::protobuf::int64 value) {
  
  id_ = value;
  // @@protoc_insertion_point(field_set:Message.id)
}

// string method = 2;
void Message::clear_method() {
  method_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
const ::std::string& Message::method() const {
  // @@protoc_insertion_point(field_get:Message.method)
  return method_.GetNoArena();
}
void Message::set_method(const ::std::string& value) {
  
  method_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:Message.method)
}
#if LANG_CXX11
void Message::set_method(::std::string&& value) {
  
  method_.SetNoArena(
    &::google::protobuf::internal::GetEmptyStringAlreadyInited(), std::move(value));
  // @@protoc_insertion_point(field_set_rvalue:Message.method)
}
#endif
void Message::set_method(const char* value) {
  
  method_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:Message.method)
}
void Message::set_method(const char* value, size_t size) {
  
  method_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:Message.method)
}
::std::string* Message::mutable_method() {
  
  // @@protoc_insertion_point(field_mutable:Message.method)
  return method_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
::std::string* Message::release_method() {
  // @@protoc_insertion_point(field_release:Message.method)
  
  return method_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
void Message::set_allocated_method(::std::string* method) {
  if (method != NULL) {
    
  } else {
    
  }
  method_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), method);
  // @@protoc_insertion_point(field_set_allocated:Message.method)
}

// bytes content = 3;
void Message::clear_content() {
  content_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
const ::std::string& Message::content() const {
  // @@protoc_insertion_point(field_get:Message.content)
  return content_.GetNoArena();
}
void Message::set_content(const ::std::string& value) {
  
  content_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:Message.content)
}
#if LANG_CXX11
void Message::set_content(::std::string&& value) {
  
  content_.SetNoArena(
    &::google::protobuf::internal::GetEmptyStringAlreadyInited(), std::move(value));
  // @@protoc_insertion_point(field_set_rvalue:Message.content)
}
#endif
void Message::set_content(const char* value) {
  
  content_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:Message.content)
}
void Message::set_content(const void* value, size_t size) {
  
  content_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:Message.content)
}
::std::string* Message::mutable_content() {
  
  // @@protoc_insertion_point(field_mutable:Message.content)
  return content_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
::std::string* Message::release_content() {
  // @@protoc_insertion_point(field_release:Message.content)
  
  return content_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
void Message::set_allocated_content(::std::string* content) {
  if (content != NULL) {
    
  } else {
    
  }
  content_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), content);
  // @@protoc_insertion_point(field_set_allocated:Message.content)
}

// int32 returnCode = 4;
void Message::clear_returncode() {
  returncode_ = 0;
}
::google::protobuf::int32 Message::returncode() const {
  // @@protoc_insertion_point(field_get:Message.returnCode)
  return returncode_;
}
void Message::set_returncode(::google::protobuf::int32 value) {
  
  returncode_ = value;
  // @@protoc_insertion_point(field_set:Message.returnCode)
}

// string returnMsg = 5;
void Message::clear_returnmsg() {
  returnmsg_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
const ::std::string& Message::returnmsg() const {
  // @@protoc_insertion_point(field_get:Message.returnMsg)
  return returnmsg_.GetNoArena();
}
void Message::set_returnmsg(const ::std::string& value) {
  
  returnmsg_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:Message.returnMsg)
}
#if LANG_CXX11
void Message::set_returnmsg(::std::string&& value) {
  
  returnmsg_.SetNoArena(
    &::google::protobuf::internal::GetEmptyStringAlreadyInited(), std::move(value));
  // @@protoc_insertion_point(field_set_rvalue:Message.returnMsg)
}
#endif
void Message::set_returnmsg(const char* value) {
  
  returnmsg_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:Message.returnMsg)
}
void Message::set_returnmsg(const char* value, size_t size) {
  
  returnmsg_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:Message.returnMsg)
}
::std::string* Message::mutable_returnmsg() {
  
  // @@protoc_insertion_point(field_mutable:Message.returnMsg)
  return returnmsg_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
::std::string* Message::release_returnmsg() {
  // @@protoc_insertion_point(field_release:Message.returnMsg)
  
  return returnmsg_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
void Message::set_allocated_returnmsg(::std::string* returnmsg) {
  if (returnmsg != NULL) {
    
  } else {
    
  }
  returnmsg_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), returnmsg);
  // @@protoc_insertion_point(field_set_allocated:Message.returnMsg)
}

#endif  // PROTOBUF_INLINE_NOT_IN_HEADERS

// @@protoc_insertion_point(namespace_scope)

// @@protoc_insertion_point(global_scope)
