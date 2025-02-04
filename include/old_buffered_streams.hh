#pragma once

/*
  This file contains old code from Themisto. In the new code we use the buffered
  streams from SBWT, but this code is retained here because these streams support
  output to standard output. At some point we should get rid of this old code and
  integrate the new functinality to the SBWT streams.
*/

#include "globals.hh"
#include "sbwt/globals.hh"
#include "sbwt/TempFileManager.hh"
#include <fstream>

namespace old_themisto_code{

// The c++ ifstream and ofstream classes are buffered. But each read involves a
// virtual function call, which can be slow if the reads or writes are in small
// chunks. The buffer is also pretty small by default. These classes store the
// input/output in a large buffer and call the stream only when the buffer is
// full, which results in far less virtual function calls and better
// performance.

class Buffered_ifstream {
   private:
    Buffered_ifstream(const Buffered_ifstream& temp_obj) =
        delete;  // No copying
    Buffered_ifstream& operator=(const Buffered_ifstream& temp_obj) =
        delete;  // No copying

    vector<char> buf;
    int64_t buf_cap = 1 << 20;

    int64_t buf_pos = 0;
    int64_t buf_size = 0;
    bool is_eof = false;
    ifstream stream;

   public:
    Buffered_ifstream(Buffered_ifstream&&) = default;             // Movable
    Buffered_ifstream& operator=(Buffered_ifstream&&) = default;  // Movable

    Buffered_ifstream() {}

    Buffered_ifstream(string filename, ios_base::openmode mode = ios_base::in)
        : stream(filename, mode) {
        if (!stream.good())
            throw std::runtime_error("Error opening file " + filename);
        buf.resize(buf_cap);
    }

    // Reads one byte to the given location.
    // Returns true if read was succesful
    bool get(char* c) {
        if (is_eof) return false;
        if (buf_pos == buf_size) {
            stream.read(buf.data(), buf_cap);
            buf_size = stream.gcount();
            buf_pos = 0;
            if (buf_size == 0) {
                is_eof = true;
                return false;
            }
        }
        *c = buf[buf_pos++];
        return true;
    }

    // Read up to n bytes to dest and returns the number of bytes read
    int64_t read(char* dest, int64_t n) {
        char* ptr = dest;
        for (int64_t i = 0; i < n; i++) {
            if (!get(ptr)) break;
            ptr++;
        }
        return ptr - dest;
    }

    bool getline(string& line) {
        line.clear();
        while (true) {
            char c;
            get(&c);
            if (eof()) return line.size() > 0;
            if (c == '\n') return true;
            line.push_back(c);
        }
    }

    // Return true if get(c) has returned false
    bool eof() { return is_eof; }

    void open(string filename, ios_base::openmode mode = ios_base::in) {
        buf.resize(buf_cap);
        stream.open(filename, mode);
        if (!stream.good())
            throw std::runtime_error("Error opening file " + filename);
        buf_size = 0;
        buf_pos = 0;
        is_eof = false;
    }

    void close() { stream.close(); }

    void set_buffer_capacity(int64_t cap) {
        this->buf_cap = cap;
        buf.resize(buf_cap);
    }
};

class Buffered_ofstream {
   private:
    Buffered_ofstream(const Buffered_ofstream& temp_obj) =
        delete;  // No copying
    Buffered_ofstream& operator=(const Buffered_ofstream& temp_obj) =
        delete;  // No copying

    vector<char> buf;
    int64_t buf_size = 0;
    int64_t buf_cap = 1 << 23;
    bool own_stream;  // Track if the internal stream was created by this object
                      // mainly to avoid trying to free cout.
    unique_ptr<ostream> stream;

    void empty_internal_buffer_to_stream() {
        if (stream.get()) {
            try {
                stream->write(buf.data(), buf_size);
            } catch (const std::ios_base::failure& e) {
                cerr << "Error writing to disk (perhaps the disk is full?)"
                     << endl;
                cerr << e.what() << endl;
                cerr << e.code() << endl;
                sbwt::get_temp_file_manager().delete_all_files();
                exit(1);
            }
            buf_size = 0;
        }
    }

   public:

    Buffered_ofstream(Buffered_ofstream&&) = default;             // Movable
    Buffered_ofstream& operator=(Buffered_ofstream&&) = default;  // Movable

    Buffered_ofstream() {}

    Buffered_ofstream(string filename,
                      ios_base::openmode mode = ios_base::out) {
        open(filename, mode);
        if (!stream->good())
            throw std::runtime_error("Error opening file " + filename);
        buf.resize(buf_cap);
    }

    Buffered_ofstream(ostream& outstream) {
        own_stream = false;
        stream.reset(&outstream);
        if (!stream->good())
            throw std::runtime_error("Error opening output stream");
        buf.resize(buf_cap);
    }

    void set_buffer_capacity(int64_t cap) {
        this->buf_cap = cap;
        buf.resize(buf_cap);
    }

    void write(const char* data, int64_t n) {
        for (int64_t i = 0; i < n; i++) {
            if (buf_cap == buf_size) empty_internal_buffer_to_stream();
            buf[buf_size++] = data[i];
        }
    }

    void open(string filename, ios_base::openmode mode = ios_base::out) {
        if (stream.get()) {
            // Flush and release ownership of previous stream if reusing
            // object
            close();
            stream.reset(new ofstream(filename, mode));
        } else {
            stream = unique_ptr<ostream>(new ofstream(filename, mode));
        }
        own_stream = true;

        buf.resize(buf_cap);
        if (!stream->good())
            throw std::runtime_error("Error opening file " + filename);
        buf_size = 0;
    }

    void open(ostream& outstream) {
        if (stream.get()) {
            // Flush and release ownership of previous stream if reusing
            // object
            close();
            stream.reset(&outstream);
        } else {
            stream = unique_ptr<ostream>(&outstream);
        }
        own_stream = false;

        buf.resize(buf_cap);
        if (!stream->good())
            throw std::runtime_error("Error opening output stream");
        buf_size = 0;
    }

    void close() {
        if (stream.get()) {
            empty_internal_buffer_to_stream();
            stream->flush();
            if (own_stream) {
                stream.reset();
            } else if (stream.get()) {
                stream.release();
            }
        }
    }

    // Flush the internal buffer AND the file stream
    void flush() {
        empty_internal_buffer_to_stream();
        stream->flush();
    }

    ~Buffered_ofstream() { close(); }
};

} // end of namespace