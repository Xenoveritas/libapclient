#ifndef _LIBAPCLIENT_CONSOLE_COMPONENT_H

#define _LIBAPCLIENT_CONSOLE_COMPONENT_H

#include <string>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include "libapclient/simple_client.h"

class ConsoleSpan {
private:
    std::string text;
    archipelago::MessageType type;
    int scrollY;
public:
    ConsoleSpan(const std::string& aText, archipelago::MessageType aType) : text(aText), type(aType), scrollY(0) {}
    const std::string& getText() const { return text; }
    archipelago::MessageType getType() const { return type; }
    void appendText(const std::string& newText);

    ftxui::Element Render();
};

class ConsoleLine {
private:
    std::vector<ConsoleSpan> spans;
public:
    ConsoleLine() : spans() {}
    // Copy constructor
    ConsoleLine(const ConsoleLine& other) = default;

    void appendText(const std::string& newText, archipelago::MessageType type);

    void clear() {
        spans.clear();
    }

    bool empty() {
        return spans.empty();
    }

    ftxui::Element Render();
};

class ConsoleComponent : public ftxui::ComponentBase {
private:
    std::vector<ConsoleLine> lines;
    // Last line is kept separate to additional text can be written to it
    ConsoleLine lastLine;
    int scrollY;
    int consoleHeight;
    ftxui::Box size;
public:
    ConsoleComponent();
    virtual ~ConsoleComponent();
    virtual ftxui::Element OnRender() override;

    virtual bool OnEvent(ftxui::Event) override;

    /*! \brief Returns true (this can be focused)
     */
    virtual bool Focusable() const override { return true; }

    /*! \brief Scroll by the given delta
     * \param scrollDelta the number of lines to scroll
     */
    void scrollByLine(int scrollDelta);

    /*! \brief Move a given number of pages
     * \param pageDelta the number of pages to move
     */
    void scrollByPage(int pageDelta);

    /*! \brief Scroll to the given line
     * \param line the line number to ensure is visible
     */
    //void scrollTo(int line);

    inline int getHeight() {
        return size.y_max - size.y_min + 1;
    }

    /*! \brief Append the given text to the end of the last line.
     *
     * \param message the message to append
     * \param type the message type (determines formatting)
     */
    void write(const std::string& message, archipelago::MessageType type = archipelago::MessageType::basic);

    /*! \brief Write a line, ending with a newline, to the client.
     *
     * \param message the message to write
     * \param type the message type (determines formatting)
     */
    void writeLn(const std::string& message = std::string(), archipelago::MessageType type = archipelago::MessageType::basic);
};

class InputWithHistory : public ftxui::ComponentBase {};

#endif // _LIBAPCLIENT_CONSOLE_COMPONENT_H