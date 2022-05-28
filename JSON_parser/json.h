#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <variant>

namespace json {
	using namespace std::literals;

	class Node;
	// Сохраните объявления Dict и Array без изменения
	using Dict = std::map<std::string, Node>;
	using Array = std::vector<Node>;

	// Эта ошибка должна выбрасываться при ошибках парсинга JSON
	class ParsingError : public std::runtime_error {
	public:
		using runtime_error::runtime_error;
	};

	class Node {
	public:
		/* Реализуйте Node, используя std::variant */
		using Value = std::variant<std::nullptr_t, int, bool, double, std::string, Array, Dict>;

		Node() = default;
		Node(std::nullptr_t value);
		Node(int value);
		Node(bool value);
		Node(double value);
		Node(std::string value);
		Node(Array array);
		Node(Dict map);

		bool IsInt() const;
		bool IsDouble() const;
		bool IsPureDouble() const;
		bool IsBool() const;
		bool IsString() const;
		bool IsNull() const;
		bool IsArray() const;
		bool IsMap() const;

		int AsInt() const;
		bool AsBool() const;
		double AsDouble() const;
		const std::string& AsString() const;
		const Array& AsArray() const;
		const Dict& AsMap() const;

		const Value& GetValue() const {
			return value_;
		}

	private:
		Value value_;
	};

	inline bool operator ==(Node left, Node right) {
		return left.GetValue() == right.GetValue();
	}

	inline bool operator !=(Node left, Node right) {
		return !(left == right);
	}

	class Document {
	public:
		explicit Document(Node root);

		const Node& GetRoot() const;

	private:
		Node root_;
	};

	inline bool operator ==(Document left, Document right) {
		return left.GetRoot() == right.GetRoot();
	}

	inline bool operator !=(Document left, Document right) {
		return !(left == right);
	}

	Document Load(std::istream& input);

	/*===========================================*/
	struct PrintContext {
		std::ostream& out;
		int indent_step = 4;
		int indent = 0;

		void PrintIndent() const {
			for (int i = 0; i < indent;++i) {
				out.put(' ');
			}
		}
		
		PrintContext Indented() const {
			return { out, indent_step, indent_step + indent };
		}
	};

	template <typename Value>
	inline void PrintValue(const Value& value, std::ostream& out) {
		out << value;
	}

	inline void PrintValue(std::nullptr_t, std::ostream& out) {
		out << "null"sv;
	}

	inline void PrintValue(const bool& value, std::ostream& out) {
		out << std::boolalpha << value;
	}

	inline void PrintValue(const std::string& value, std::ostream& out) {
		//out << value;
		out << "\""sv;
		for (const char c : value) {
			if (c == '\r') {
				out << "\\r"sv;
			}
			else if (c == '\n') {
				out << "\\n"sv;
			}
			else if (c == '\\'|| c =='\"') {
				out << "\\"sv;
				out << c;
			}
			else {
				out << c;
			}
			
		}
		out << "\""sv;
	}

	void PrintNode(const Node&, std::ostream&);
	inline void PrintValue(const Array& array, std::ostream& out) {
		/*for (const auto& value : array) {
			PrintNode(value, out);
		}*/
		out << "["sv;
		for (size_t i = 0; i < array.size(); ++i) {
			if (i > 0) { out << ","sv; }
			PrintNode(array[i], out);
		}
		out << "]"sv;
	}
	inline void PrintValue(const Dict& dict, std::ostream& out) {
		out << "{"sv;
		for (const auto& [key, value] : dict) {
			PrintNode(key, out);
			out << ":"sv;
			PrintNode(value, out);
		}
		out << "}"sv;
	}
	/*int, bool, double, std::string, Array, Dict*/
	inline void PrintNode(const Node& node, std::ostream& out) {
		std::visit(
			[&out](const auto& value) { PrintValue(value, out);},
			node.GetValue());
	}
	/*===========================================*/

	void Print(const Document& doc, std::ostream& output);

}  // namespace json