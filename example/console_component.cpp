/*! \file console_component.cpp
 * \brief a scrollable "console" in FTXUI
 */

#include <algorithm>
#include <utility>

#include <ftxui/component/app.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>
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

ConsoleComponent::ConsoleComponent() : lines(), lastLine(), scrollY(0), consoleHeight(0), size() {}
ConsoleComponent::~ConsoleComponent() {
}

ftxui::Element ConsoleComponent::OnRender() {
    ftxui::Elements children;
    // Make sure it's large enough
    children.reserve(lines.size());
    for (auto& line : lines) {
        children.push_back(line.Render());
    }
    auto consoleElement = ftxui::vbox(children);
    consoleElement->ComputeRequirement();
    consoleHeight = consoleElement->requirement().min_y;
    return ftxui::dbox({
        std::move(consoleElement),
        ftxui::vbox({
            // Padding to move the selected element down
            ftxui::text("") | ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, scrollY),
            // focused element to scroll to
            ftxui::text("") | ftxui::focus
        })
    })
        | ftxui::vscroll_indicator
        | ftxui::yframe
        | ftxui::yflex
        | ftxui::reflect(size);
}

bool ConsoleComponent::OnEvent(ftxui::Event event) {
    if (event.is_mouse()) {
        const ftxui::Mouse& mouse = event.mouse();
        if (mouse.motion == ftxui::Mouse::Pressed) {
            switch (mouse.button) {
                case ftxui::Mouse::Left:
                    TakeFocus();
                    break;
                case ftxui::Mouse::WheelUp:
                    scrollByLine(-1);
                    break;
                case ftxui::Mouse::WheelDown:
                    scrollByLine(1);
                    break;
                default:
                    // Intentionally ignore
                    break;
            }
        }
    }
    if (event == ftxui::Event::ArrowUp) {
        // Scroll up
        scrollByLine(-1);
        return true;
    }
    if (event == ftxui::Event::ArrowDown) {
        scrollByLine(1);
        return true;
    }
    if (event == ftxui::Event::PageUp) {
        scrollByPage(-1);
        return true;
    }
    if (event == ftxui::Event::PageDown) {
        scrollByPage(1);
        return true;
    }
    return false;
}

void ConsoleComponent::scrollByLine(int scrollDelta) {
    scrollY += scrollDelta;
    scrollY = std::max(0, std::min(scrollY, consoleHeight - 1));
    LIBAPCLIENT_LOG("Scroll: {:d} Console Height: {:d} Min Y: {:d} Min X: {:d}", scrollY, getHeight(), size.y_min, size.y_max);
}

void ConsoleComponent::scrollByPage(int pageDelta) {
    // For now:
    scrollByLine(pageDelta * (size.y_max - size.y_min));
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
    scrollY = consoleHeight - 1;
}