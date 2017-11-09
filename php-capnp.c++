#include <capnp/message.h>
#include <capnp/serialize.h>
#include <iostream>
#include <capnp/schema.h>
#include <capnp/dynamic.h>

#include <capnp/schema-parser.h>

#include <algorithm>

#include <phpcpp.h>

using ::capnp::DynamicValue;
using ::capnp::DynamicStruct;
using ::capnp::DynamicEnum;
using ::capnp::DynamicList;
using ::capnp::List;
using ::capnp::Schema;
using ::capnp::StructSchema;
using ::capnp::EnumSchema;

using ::capnp::Void;
using ::capnp::Text;
using ::capnp::FlatArrayMessageReader;

using ::capnp::SchemaParser;
using ::capnp::ParsedSchema;

SchemaParser parser;
ParsedSchema parsed;
StructSchema schema;

std::string to_underscore(std::string& input) {
	size_t count_upper = count_if(input.begin(), input.end(),
		[](unsigned char ch) { return isupper(ch); });

	std::string s;
	s.reserve(input.length() + count_upper);

	for(char c : input) {
		if(isupper(c))
			s += '_';

		s += (char)tolower(c);
	}

	return s;
}

Php::Value dynamicValue(DynamicValue::Reader value) {
	switch (value.getType()) {
		case DynamicValue::VOID:
			break;
		case DynamicValue::BOOL:
			return value.as<bool>();
			break;
		case DynamicValue::INT:
			return value.as<int64_t>();
			break;
		case DynamicValue::UINT:
			return value.as<int64_t>(); // TODO: uint64_t?
			break;
		case DynamicValue::FLOAT:
			return value.as<double>();
			break;
		case DynamicValue::TEXT:
			return value.as<Text>().cStr();
			break;
		case DynamicValue::LIST: {
			Php::Value _array;
			for (auto element: value.as<DynamicList>()) {
				Php::array_push(_array, dynamicValue(element));
			}
			return _array;
			break;
		}
		case DynamicValue::ENUM: {
			auto enumValue = value.as<DynamicEnum>();
			KJ_IF_MAYBE(enumerant, enumValue.getEnumerant()) {
				return enumerant->getProto().getName().cStr();
			} else {
				// Unknown enum value; throw error?
			}
			break;
		}
		case DynamicValue::STRUCT: {
			auto structValue = value.as<DynamicStruct>();
			Php::Value _struct;
			for (auto field: structValue.getSchema().getFields()) {
				if (!structValue.has(field))
					continue;

				std::string var_name = field.getProto().getName().cStr();
				var_name = to_underscore(var_name);

				_struct[var_name] = dynamicValue(structValue.get(field));
			}
			return _struct;
			break;
		}
		default:
			break;
	}

	return nullptr;
}

Php::Value php_capnp(Php::Parameters &parameters) {
	std::string message = parameters[0].stringValue();

	kj::ArrayPtr<const capnp::word> buff(
		(const capnp::word*)message.c_str(),
		message.size() / sizeof(capnp::word)
	);

	try {
		return dynamicValue(FlatArrayMessageReader(buff).getRoot<DynamicStruct>(schema));
	} catch(const kj::Exception& ex) {
		throw Php::Exception(ex.getDescription());
	}
}

extern "C" {
	PHPCPP_EXPORT void *get_module() {
		static Php::Extension extension("php_capnp", "0.1");
		extension.add<php_capnp>("capnp");

		extension.add(Php::Ini("php_capnp.schema", ""));
		extension.add(Php::Ini("php_capnp.object", ""));

		extension.onStartup([]() {
			std::string capnp_schema = Php::ini_get("php_capnp.schema");
			std::string capnp_object = Php::ini_get("php_capnp.object");

			try {
				parsed = parser.parseDiskFile("php_capnp", capnp_schema, {""});
				schema = (StructSchema) parsed.getNested(capnp_object).asStruct();
			} catch (const kj::Exception& ex) {
				throw Php::Exception(ex.getDescription());
			}
		});

		return extension;
	}
}
