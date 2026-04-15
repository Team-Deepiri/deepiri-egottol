#include "sdr_buffer.h"
#include <cstring>
#include <algorithm>

namespace deepiri {

class SDRBuffer::Impl {
public:
    std::vector<std::complex<float>> buffer_;
    size_t read_pos_;
    size_t write_pos_;
    size_t count_;
    SDRFormat format_;
    SDRMetadata metadata_;
    size_t capacity_;

    Impl() : read_pos_(0), write_pos_(0), count_(0), format_(SDRFormat::CF32), capacity_(8192) {
        buffer_.reserve(capacity_);
    }

    void convertFromCS16(const int16_t* input, size_t samples) {
        buffer_.clear();
        for (size_t i = 0; i < samples; ++i) {
            float re = static_cast<float>(input[i * 2]) / 32768.0f;
            float im = static_cast<float>(input[i * 2 + 1]) / 32768.0f;
            buffer_.push_back({re, im});
        }
    }

    void convertFromCU8(const uint8_t* input, size_t samples) {
        buffer_.clear();
        for (size_t i = 0; i < samples; ++i) {
            float re = (static_cast<float>(input[i * 2]) - 128.0f) / 128.0f;
            float im = (static_cast<float>(input[i * 2 + 1]) - 128.0f) / 128.0f;
            buffer_.push_back({re, im});
        }
    }

    void advanceRead(size_t count) {
        read_pos_ = (read_pos_ + count) % capacity_;
        count_ = std::max(size_t(0), count_ - count);
    }

    void advanceWrite(size_t count) {
        write_pos_ = (write_pos_ + count) % capacity_;
        count_ += count;
    }
};

SDRBuffer::SDRBuffer() : pImpl(std::make_unique<Impl>()) {}
SDRBuffer::~SDRBuffer() = default;

void SDRBuffer::setFormat(SDRFormat format) {
    pImpl->format_ = format;
}

void SDRBuffer::setCapacity(size_t max_samples) {
    pImpl->capacity_ = max_samples;
    pImpl->buffer_.reserve(max_samples);
}

bool SDRBuffer::write(const std::vector<std::complex<float>>& samples) {
    if (samples.size() > pImpl->capacity_) {
        return false;
    }

    pImpl->buffer_.insert(pImpl->buffer_.end(), samples.begin(), samples.end());
    pImpl->advanceWrite(samples.size());
    return true;
}

bool SDRBuffer::read(size_t count, std::vector<std::complex<float>>& output) {
    if (count > pImpl->count_) {
        count = pImpl->count_;
    }

    output.clear();
    size_t end_pos = pImpl->read_pos_ + count;
    if (end_pos <= pImpl->buffer_.size()) {
        output.insert(output.end(), pImpl->buffer_.begin() + pImpl->read_pos_, pImpl->buffer_.begin() + end_pos);
    } else {
        size_t first_part = pImpl->buffer_.size() - pImpl->read_pos_;
        output.insert(output.end(), pImpl->buffer_.begin() + pImpl->read_pos_, pImpl->buffer_.end());
        output.insert(output.end(), pImpl->buffer_.begin(), pImpl->buffer_.begin() + (count - first_part));
    }

    pImpl->advanceRead(count);
    return true;
}

size_t SDRBuffer::available() const {
    return pImpl->count_;
}

size_t SDRBuffer::capacity() const {
    return pImpl->capacity_;
}

void SDRBuffer::clear() {
    pImpl->buffer_.clear();
    pImpl->read_pos_ = 0;
    pImpl->write_pos_ = 0;
    pImpl->count_ = 0;
}

void SDRBuffer::setMetadata(const SDRMetadata& meta) {
    pImpl->metadata_ = meta;
}

SDRMetadata SDRBuffer::getMetadata() const {
    return pImpl->metadata_;
}

SDRFormat SDRBuffer::getFormat() const {
    return pImpl->format_;
}

}