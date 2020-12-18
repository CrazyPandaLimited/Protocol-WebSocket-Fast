// this file is included into struct Parser
// ! no namespaces or #includes here !

struct FrameSender {
    FrameSender (FrameSender&& other) : _parser(other._parser) {}
    FrameSender (FrameSender&) = delete;

    string send (IsFinal final = IsFinal::NO)                      { return _parser.send_frame(final); }
    string send (string_view payload, IsFinal final = IsFinal::NO) { return _parser.send_frame(payload, final); }

    template <typename It, class T = decltype(*std::declval<It>()), class = std::enable_if_t<std::is_convertible<T, string_view>::value>>
    string send (It&& payload_begin, It&& payload_end, IsFinal final = IsFinal::NO) {
        return _parser.send_frame(std::forward<It>(payload_begin), std::forward<It>(payload_end), final);
    }

protected:
    FrameSender (Parser& parser) : _parser(parser) {}

    Parser& _parser;

private:
    friend Parser;
};
