// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <klex/SourceLocation.h>
#include <fmt/format.h>

#include <algorithm>
#include <functional>
#include <string>
#include <system_error>
#include <vector>

namespace klex {

class Report {
  public:
	enum class Type { TokenError, SyntaxError, TypeError, Warning, LinkError };

	struct Message {
		Type type;
		SourceLocation sourceLocation;
		std::string text;

		Message(Type _type, SourceLocation _sloc, std::string _text)
			: type{_type}, sourceLocation{_sloc}, text{_text}
		{
		}

		std::string to_string() const;

		bool operator==(const Message& other) const noexcept;
		bool operator!=(const Message& other) const noexcept { return !(*this == other); }
	};

	using MessageList = std::vector<Message>;
	using Reporter = std::function<void(Message)>;

	explicit Report(Reporter reporter) : onReport_{std::move(reporter)} {}

	template <typename... Args>
	void tokenError(const SourceLocation& sloc, const std::string& f, Args... args)
	{
		report(Type::TokenError, sloc, fmt::format(f, std::move(args)...));
	}

	template <typename... Args>
	void syntaxError(const SourceLocation& sloc, const std::string& f, Args... args)
	{
		report(Type::SyntaxError, sloc, fmt::format(f, std::move(args)...));
	}

	template <typename... Args>
	void typeError(const SourceLocation& sloc, const std::string& f, Args... args)
	{
		report(Type::TypeError, sloc, fmt::format(f, std::move(args)...));
	}

	template <typename... Args>
	void warning(const SourceLocation& sloc, const std::string& f, Args... args)
	{
		report(Type::Warning, sloc, fmt::format(f, std::move(args)...));
	}

	template <typename... Args>
	void linkError(const std::string& f, Args... args)
	{
		report(Type::LinkError, SourceLocation{}, fmt::format(f, std::move(args)...));
	}

	void report(Type type, SourceLocation sloc, std::string text)
	{
		if (type != Type::Warning)
			errorCount_++;

		if (onReport_)
		{
			onReport_(Message(type, std::move(sloc), std::move(text)));
		}
	}

	bool containsFailures() const noexcept { return errorCount_ != 0; }

  private:
	size_t errorCount_ = 0;
	Reporter onReport_;
};

class ConsoleReport : public Report {
  public:
	ConsoleReport() : Report(std::bind(&ConsoleReport::onMessage, this, std::placeholders::_1)) {}

  private:
	void onMessage(Message&& msg);
};

class BufferedReport : public Report {
  public:
	BufferedReport() : Report(std::bind(&BufferedReport::onMessage, this, std::placeholders::_1)), messages_{}
	{
	}

	std::string to_string() const;

	const MessageList& messages() const noexcept { return messages_; }

	void clear();
	size_t size() const noexcept { return messages_.size(); }
	const Message& operator[](size_t i) const { return messages_[i]; }

	using iterator = MessageList::iterator;
	using const_iterator = MessageList::const_iterator;

	iterator begin() noexcept { return messages_.begin(); }
	iterator end() noexcept { return messages_.end(); }
	const_iterator begin() const noexcept { return messages_.begin(); }
	const_iterator end() const noexcept { return messages_.end(); }

	bool contains(const Message& m) const noexcept;

	bool operator==(const BufferedReport& other) const noexcept;
	bool operator!=(const BufferedReport& other) const noexcept { return !(*this == other); }

  private:
	void onMessage(Message&& msg);

  private:
	MessageList messages_;
};

std::ostream& operator<<(std::ostream& os, const BufferedReport& report);

using DifferenceReport = std::pair<Report::MessageList, Report::MessageList>;

DifferenceReport difference(const BufferedReport& first, const BufferedReport& second);

}  // namespace klex

namespace fmt {
template <>
struct formatter<klex::Report::Type> {
	using Type = klex::Report::Type;

	template <typename ParseContext>
	constexpr auto parse(ParseContext& ctx)
	{
		return ctx.begin();
	}

	template <typename FormatContext>
	constexpr auto format(const Type& v, FormatContext& ctx)
	{
		switch (v)
		{
			case Type::TokenError:
				return format_to(ctx.begin(), "TokenError");
			case Type::SyntaxError:
				return format_to(ctx.begin(), "SyntaxError");
			case Type::TypeError:
				return format_to(ctx.begin(), "TypeError");
			case Type::Warning:
				return format_to(ctx.begin(), "Warning");
			case Type::LinkError:
				return format_to(ctx.begin(), "LinkError");
			default:
				return format_to(ctx.begin(), "{}", static_cast<unsigned>(v));
		}
	}
};
}  // namespace fmt

namespace fmt {
template <>
struct formatter<klex::SourceLocation> {
	template <typename ParseContext>
	constexpr auto parse(ParseContext& ctx)
	{
		return ctx.begin();
	}

	template <typename FormatContext>
	constexpr auto format(const klex::SourceLocation& sloc, FormatContext& ctx)
	{
		return format_to(ctx.begin(), "{} ({}-{})", sloc.filename, sloc.offset, sloc.offset + sloc.count);
	}
};
}  // namespace fmt

namespace fmt {
template <>
struct formatter<klex::Report::Message> {
	using Message = klex::Report::Message;

	template <typename ParseContext>
	constexpr auto parse(ParseContext& ctx)
	{
		return ctx.begin();
	}

	template <typename FormatContext>
	constexpr auto format(const Message& v, FormatContext& ctx)
	{
		return format_to(ctx.begin(), "{}", v.to_string());
	}
};
}  // namespace fmt
