// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#include "node_binder/v8/converter.h"

#include <stdint.h>

#include "node/v8.h"
#include "base/strings/string_number_conversions.h"

using v8::ArrayBuffer;
using v8::Boolean;
using v8::External;
using v8::Function;
using v8::Int32;
using v8::Integer;
using v8::Isolate;
using v8::Local;
using v8::Maybe;
using v8::MaybeLocal;
using v8::Number;
using v8::Object;
using v8::String;
using v8::Uint32;
using v8::Value;

namespace {

template <typename T, typename U>
bool FromMaybe(Maybe<T> maybe, U* out) {
  if (maybe.IsNothing())
    return false;
  *out = static_cast<U>(maybe.FromJust());
  return true;
}

}  // namespace

namespace v8 {

Local<Value> Converter<bool>::ToV8(Isolate* isolate, bool val) {
  return Boolean::New(isolate, val).As<Value>();
}

bool Converter<bool>::FromV8(Isolate* isolate, Local<Value> val, bool* out) {
  return FromMaybe(val->BooleanValue(isolate->GetCurrentContext()), out);
}

Local<Value> Converter<int32_t>::ToV8(Isolate* isolate, int32_t val) {
  return Integer::New(isolate, val).As<Value>();
}

bool Converter<int32_t>::FromV8(Isolate* isolate,
                                Local<Value> val,
                                int32_t* out) {
  if (!val->IsInt32())
    return false;
  *out = val.As<Int32>()->Value();
  return true;
}

Local<Value> Converter<uint32_t>::ToV8(Isolate* isolate, uint32_t val) {
  return Integer::NewFromUnsigned(isolate, val).As<Value>();
}

bool Converter<uint32_t>::FromV8(Isolate* isolate,
                                 Local<Value> val,
                                 uint32_t* out) {
  if (!val->IsUint32())
    return false;
  *out = val.As<Uint32>()->Value();
  return true;
}

Local<Value> Converter<int64_t>::ToV8(Isolate* isolate, int64_t val) {
  return Number::New(isolate, static_cast<double>(val)).As<Value>();
}

bool Converter<int64_t>::FromV8(Isolate* isolate,
                                Local<Value> val,
                                int64_t* out) {
  if (!val->IsNumber())
    return false;
  // Even though IntegerValue returns int64_t, JavaScript cannot represent
  // the full precision of int64_t, which means some rounding might occur.
  return FromMaybe(val->IntegerValue(isolate->GetCurrentContext()), out);
}

Local<Value> Converter<uint64_t>::ToV8(Isolate* isolate, uint64_t val) {
  return Number::New(isolate, static_cast<double>(val)).As<Value>();
}

bool Converter<uint64_t>::FromV8(Isolate* isolate,
                                 Local<Value> val,
                                 uint64_t* out) {
  if (!val->IsNumber())
    return false;
  return FromMaybe(val->IntegerValue(isolate->GetCurrentContext()), out);
}

Local<Value> Converter<float>::ToV8(Isolate* isolate, float val) {
  return Number::New(isolate, val).As<Value>();
}

bool Converter<float>::FromV8(Isolate* isolate, Local<Value> val, float* out) {
  if (!val->IsNumber())
    return false;
  *out = static_cast<float>(val.As<Number>()->Value());
  return true;
}

Local<Value> Converter<double>::ToV8(Isolate* isolate, double val) {
  return Number::New(isolate, val).As<Value>();
}

bool Converter<double>::FromV8(Isolate* isolate,
                               Local<Value> val,
                               double* out) {
  if (!val->IsNumber())
    return false;
  *out = val.As<Number>()->Value();
  return true;
}

Local<Value> Converter<std::string>::ToV8(Isolate* isolate,
                                          const std::string& val) {
  return String::NewFromUtf8(isolate, val.data(),
                             v8::NewStringType::kNormal,
                             static_cast<uint32_t>(val.length()))
      .ToLocalChecked();
}

Local<Value> Converter<std::string>::ToV8(Isolate* isolate,
                                          const char* var, size_t len) {
  return String::NewFromUtf8(isolate, var, v8::NewStringType::kNormal,
                             static_cast<uint32_t>(len))
      .ToLocalChecked();
}

bool Converter<std::string>::FromV8(Isolate* isolate,
                                    Local<Value> val,
                                    std::string* out) {
  if (!val->IsString())
    return false;
  Local<String> str = Local<String>::Cast(val);
  int length = str->Utf8Length();
  out->resize(length);
  str->WriteUtf8(&(*out)[0], length, NULL, String::NO_NULL_TERMINATION);
  return true;
}

Local<Value> Converter<net::SpdyHeaderBlock>::ToV8(
    Isolate* isolate,
    const net::SpdyHeaderBlock& val) {
  EscapableHandleScope scope(isolate);
  Local<Object> out = Object::New(isolate);
  for (net::SpdyHeaderBlock::const_iterator it = val.begin();
       it != val.end(); ++it) {
    out->Set(Converter<std::string>::ToV8(isolate, it->first.as_string()),
             Converter<std::string>::ToV8(isolate, it->second.as_string()));
  }
  return scope.Escape(out);
}

bool Converter<net::SpdyHeaderBlock>::FromV8(Isolate* isolate,
                                             Local<Value> val,
                                             net::SpdyHeaderBlock* out) {
  Local<Object> headers = Local<Object>::Cast(val);
  Local<Array> header_names = headers->GetOwnPropertyNames();
  for (size_t i = 0; i < header_names->Length(); ++i) {
    Local<Value> key = header_names->Get(i);
    Local<Value> value = headers->Get(key);
    std::string header_name = V8ToString(key);
    std::string header_value;
    if (value->IsString()) {
      header_value = V8ToString(value);
    } else if (value->IsNumber()) {
      double number_value = 0.0;
      Converter<double>::FromV8(isolate, value, &number_value);
      header_value = base::DoubleToString(number_value);
    }

    out->AppendValueOrAddHeader(header_name, header_value);
  }
  return true;
}

v8::Local<v8::Value> Converter<net::HttpResponseHeaders>::ToV8(
    v8::Isolate* isolate,
    const net::HttpResponseHeaders& val) {
  EscapableHandleScope scope(isolate);
  std::string name;
  std::string value;
  Local<Object> out = Object::New(isolate);
  for (size_t it = 0; val.EnumerateHeaderLines(&it, &name, &value);) {
    out->Set(Converter<std::string>::ToV8(isolate, name),
             Converter<std::string>::ToV8(isolate, value));
  }
  return scope.Escape(out);
}

bool Converter<net::HttpResponseHeaders>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    net::HttpResponseHeaders* out) {
  Local<Object> headers = Local<Object>::Cast(val);
  Local<Array> header_names = headers->GetOwnPropertyNames();
  std::stringstream raw_headers;
  for (size_t i = 0; i < header_names->Length(); ++i) {
    Local<Value> key = header_names->Get(i);
    Local<Value> value = headers->Get(key);
    std::string header_name = V8ToString(key);
    std::string header_value;
    if (value->IsString()) {
      header_value = V8ToString(value);
    } else if (value->IsNumber()) {
      double number_value = 0.0;
      Converter<double>::FromV8(isolate, value, &number_value);
      header_value = base::DoubleToString(number_value);
    }
    raw_headers << header_name << ": " << header_value << "\r\n";
  }

  raw_headers << "\r\n";
  net::HttpResponseHeaders* source =
      new net::HttpResponseHeaders(raw_headers.str());

  out->Update(*source);

  return true;
}

bool Converter<Local<Function>>::FromV8(Isolate* isolate,
                                        Local<Value> val,
                                        Local<Function>* out) {
  if (!val->IsFunction())
    return false;
  *out = Local<Function>::Cast(val);
  return true;
}

Local<Value> Converter<Local<Object>>::ToV8(Isolate* isolate,
                                            Local<Object> val) {
  return val.As<Value>();
}

bool Converter<Local<Object>>::FromV8(Isolate* isolate,
                                      Local<Value> val,
                                      Local<Object>* out) {
  if (!val->IsObject())
    return false;
  *out = Local<Object>::Cast(val);
  return true;
}

Local<Value> Converter<Local<ArrayBuffer>>::ToV8(Isolate* isolate,
                                                 Local<ArrayBuffer> val) {
  return val.As<Value>();
}

bool Converter<Local<ArrayBuffer>>::FromV8(Isolate* isolate,
                                           Local<Value> val,
                                           Local<ArrayBuffer>* out) {
  if (!val->IsArrayBuffer())
    return false;
  *out = Local<ArrayBuffer>::Cast(val);
  return true;
}

Local<Value> Converter<Local<External>>::ToV8(Isolate* isolate,
                                              Local<External> val) {
  return val.As<Value>();
}

bool Converter<Local<External>>::FromV8(Isolate* isolate,
                                        v8::Local<Value> val,
                                        Local<External>* out) {
  if (!val->IsExternal())
    return false;
  *out = Local<External>::Cast(val);
  return true;
}

Local<Value> Converter<Local<Value>>::ToV8(Isolate* isolate, Local<Value> val) {
  return val;
}

bool Converter<Local<Value>>::FromV8(Isolate* isolate,
                                     Local<Value> val,
                                     Local<Value>* out) {
  *out = val;
  return true;
}

v8::Local<v8::String> StringToSymbol(v8::Isolate* isolate,
                                     const std::string& val) {
  return String::NewFromUtf8(isolate, val.data(),
                             v8::NewStringType::kInternalized,
                             static_cast<uint32_t>(val.length()))
      .ToLocalChecked();
}

std::string V8ToString(v8::Local<v8::Value> value) {
  if (value.IsEmpty())
    return std::string();
  std::string result;
  if (!ConvertFromV8(NULL, value, &result))
    return std::string();
  return result;
}

}  // namespace v8

