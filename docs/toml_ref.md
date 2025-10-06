// --- toml++ 3.4.0 Minimal API Reference ---

#pragma once
#include <string>
#include <string_view>
#include <optional>
#include <vector>
#include <map>
#include <memory>
#include <iostream>

namespace toml {

// --- Basic Types ---

enum class node_type : uint8_t {
    none, table, array, string, integer, floating_point,
    boolean, date, time, date_time
};

// Chrono types
struct date { uint16_t year; uint8_t month, day; };
struct time { uint8_t hour, minute, second; uint32_t nanosecond; };
struct time_offset { int16_t minutes; };
struct date_time { date date; time time; std::optional<time_offset> offset; };

// --- Core Node Classes ---

class node {
public:
    virtual ~node() noexcept;
    virtual node_type type() const noexcept = 0;
    virtual bool is_table() const noexcept = 0;
    virtual bool is_array() const noexcept = 0;
    virtual bool is_value() const noexcept = 0;
    
    template<typename T>
    bool is() const noexcept; // Check if node matches T

    template<typename T>
    class value<T>* as() noexcept; // cast to concrete value type or container

    template<typename T>
    std::optional<T> value() const noexcept; // get value if type matches

    // Navigation
    virtual node* as_table() noexcept = 0;
    virtual class table* as_table() const noexcept = 0;
    virtual class array* as_array() noexcept = 0;
    virtual class array* as_array() const noexcept = 0;

    node& operator[](const std::string_view& key) noexcept;

    // etc...
};

template<typename ValueType>
class value : public node {
public:
    explicit value(const ValueType& val);
    ValueType& get() noexcept;
    const ValueType& get() const noexcept;
    // Comparison operators (==, !=)
    // etc...
};

class table : public node {
public:
    table();
    ~table() noexcept;

    bool is_table() const noexcept override { return true; }
    bool is_array() const noexcept override { return false; }
    bool is_value() const noexcept override { return false; }

    node* get(const std::string_view& key) noexcept;
    node& at(const std::string_view& key);
    std::pair<table::iterator, bool> insert_or_assign(std::string_view key, node* val);

    using iterator = std::map<std::string, std::unique_ptr<node>>::iterator;
    iterator begin() noexcept;
    iterator end() noexcept;

    void clear() noexcept;
    bool empty() const noexcept;
    size_t size() const noexcept;
};

class array : public node {
public:
    array();
    ~array() noexcept;

    bool is_table() const noexcept override { return false; }
    bool is_array() const noexcept override { return true; }
    bool is_value() const noexcept override { return false; }

    node* get(size_t index) noexcept;
    node& at(size_t index);

    void push_back(node* val);
    void clear() noexcept;
    size_t size() const noexcept;

    using iterator = std::vector<std::unique_ptr<node>>::iterator;
    iterator begin() noexcept;
    iterator end() noexcept;
};

// --- Parsing ---

class parse_error : public std::runtime_error {
public:
    parse_error(std::string_view desc);
    std::string_view description() const noexcept;
};

struct parse_result {
    bool succeeded() const noexcept;
    table& table() & noexcept;
    const table& table() const & noexcept;
    parse_error& error() & noexcept;
};

parse_result parse(std::string_view toml_text, std::string_view source_path = {});
parse_result parse_file(std::string_view file_path);

// --- Path ---

class path {
public:
    path() noexcept;
    explicit path(std::string_view);
    std::string str() const;
    void clear() noexcept;

    size_t size() const noexcept;
    bool empty() const noexcept;

    // concatenation, append, prepend...
};

node* at_path(node& root, const path& p) noexcept;
node* at_path(node& root, std::string_view path) noexcept;

// --- Utilities ---

template <typename T>
std::optional<T> value(const node& node) noexcept;

template <typename T>
value<T>* as(node* n) noexcept;

// --- Formatters (dumping) ---

class toml_formatter {
public:
    toml_formatter(const node& root);
    friend std::ostream& operator<<(std::ostream&, const toml_formatter&);
};

class json_formatter {
public:
    json_formatter(const node& root);
    friend std::ostream& operator<<(std::ostream&, const json_formatter&);
};

class yaml_formatter {
public:
    yaml_formatter(const node& root);
    friend std::ostream& operator<<(std::ostream&, const yaml_formatter&);
};

} // namespace toml
