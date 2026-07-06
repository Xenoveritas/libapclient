/*! \file console_component.cpp
 * \brief a scrollable "console" in FTXUI
 */

#include <ftxui/component/app.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

#include "libapclient/simple_client.h"
#include "libapclient/logger.h"

#include "console_component.h"

void ConsoleSpan::appendText(const std::string& newText) {
    text.append(newText);
}

ftxui::Element ConsoleSpan::Render() {
    auto element = ftxui::paragraph(text);
    switch (type) {
    case archipelago::MessageType::basic:
        break;
    case archipelago::MessageType::error:
        element |= ftxui::color(ftxui::Color::Red);
        break;
    case archipelago::MessageType::help:
        element |= ftxui::color(ftxui::Color::BlueLight);
        break;
    case archipelago::MessageType::server:
        break;
    }
    return element;
}

void ConsoleLine::appendText(const std::string& newText, archipelago::MessageType type) {
    if (spans.empty()) {
        // Just add a new span
        spans.push_back(std::move(ConsoleSpan(newText, type)));
    } else {
        // Check if the back span is the same type
        ConsoleSpan& existing = spans.back();
        if (existing.getType() == type) {
            existing.appendText(newText);
        } else {
            // Otherwise, make a new span
            spans.push_back(std::move(ConsoleSpan(newText, type)));
        }
    }
}

ftxui::Element ConsoleLine::Render() {
    if (spans.empty()) {
        return ftxui::text("");
    }
    if (spans.size() == 1) {
        return spans.front().Render();
    }
    // Otherwise, build a vbox
    ftxui::Elements elements;
    elements.reserve(spans.size());
    for (auto& span : spans) {
        elements.push_back(span.Render());
    }
    return ftxui::hflow(elements);
}

ConsoleComponent::ConsoleComponent() : lines(), lastLine(), scrollY(0), size() {}
ConsoleComponent::~ConsoleComponent() {
}

ftxui::Element ConsoleComponent::OnRender() {
    ftxui::Elements children;
    // Make sure it's large enough
    children.reserve(lines.size());
    for (auto& line : lines) {
        children.push_back(line.Render());
    }
    return ftxui::flexbox(children, {
        .direction = ftxui::FlexboxConfig::Direction::Column
    })
        | ftxui::vscroll_indicator
        | ftxui::focusPosition(0, scrollY)
        | ftxui::yframe
        | ftxui::flex
        | ftxui::reflect(size);
}

bool ConsoleComponent::OnEvent(ftxui::Event event) {
    if (event == ftxui::Event::ArrowUp) {
        // Scroll up
        scrollBy(-1);
        return true;
    }
    if (event == ftxui::Event::ArrowDown) {
        scrollBy(1);
        return true;
    }
    if (event == ftxui::Event::PageUp) {
        scrollBy(-10);
        return true;
    }
    if (event == ftxui::Event::PageDown) {
        scrollBy(10);
        return true;
    }
    return false;
}

void ConsoleComponent::scrollBy(int scrollDelta) {
    scrollY += scrollDelta;
    // There's always a single line (the last)
    int lineHeight = lines.size() + 1;
    // Actual max is whatever our line count is - height
    int min = lineHeight - size.y_max - size.y_min;
    if (scrollY < min) {
        scrollY = min;
    }
    if (scrollY > lineHeight) {
        scrollY = lineHeight;
    }
}

void ConsoleComponent::scrollTo(int line) {
    // For now:
    scrollY = line + size.y_max - size.y_min;
}

void ConsoleComponent::write(const std::string& message, archipelago::MessageType type) {
    lastLine.appendText(message, type);
}

void ConsoleComponent::writeLn(const std::string& message, archipelago::MessageType type) {
    // Write the current message
    write(message, type);
    // Append (and copy) our last line to the static lines
    lines.push_back(lastLine);
    // Clear the last line
    lastLine.clear();
    // Scroll it into view
    scrollTo(lines.size());
}