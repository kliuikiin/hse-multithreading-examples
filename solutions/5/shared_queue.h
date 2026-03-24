#pragma once

#include <atomic>
#include <cstdint>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <vector>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

static constexpr uint32_t PROTOCOL_VERSION = 1;
static constexpr uint32_t SLOT_COUNT       = 1024;
static constexpr uint32_t DATA_SIZE        = 1024 * 1024;  // 1MB payload buffer

static_assert(std::atomic<uint32_t>::is_always_lock_free);
static_assert(std::atomic<uint8_t>::is_always_lock_free);

struct MessageHeader {
    uint32_t type;
    uint32_t length;
};

struct Message {
    MessageHeader header;
    std::vector<uint8_t> payload;
};

struct Slot {
    std::atomic<uint8_t> ready{0};
    uint32_t type;
    uint32_t length;
    uint32_t data_offset;
};

struct SharedMemory {
    uint32_t version;
    std::atomic<uint32_t> slot_tail{0};
    std::atomic<uint32_t> slot_head{0};
    std::atomic<uint32_t> data_tail{0};
    Slot slots[SLOT_COUNT];
    uint8_t data_buffer[DATA_SIZE];
};

static void WriteData(uint8_t* buf, uint32_t offset, const void* src, uint32_t len) {
    uint32_t start = offset % DATA_SIZE;
    uint32_t first = std::min(len, DATA_SIZE - start);
    std::memcpy(buf + start, src, first);
    if (first < len)
        std::memcpy(buf, static_cast<const uint8_t*>(src) + first, len - first);
}

static void ReadData(const uint8_t* buf, uint32_t offset, void* dst, uint32_t len) {
    uint32_t start = offset % DATA_SIZE;
    uint32_t first = std::min(len, DATA_SIZE - start);
    std::memcpy(dst, buf + start, first);
    if (first < len)
        std::memcpy(static_cast<uint8_t*>(dst) + first, buf, len - first);
}

class ProducerNode {
public:
    ProducerNode(const char* name) : name_(name) {
        fd_ = shm_open(name, O_CREAT | O_RDWR, 0666);
        if (fd_ == -1) throw std::runtime_error("shm_open failed");
        if (ftruncate(fd_, sizeof(SharedMemory)) == -1) throw std::runtime_error("ftruncate failed");
        shm_ = static_cast<SharedMemory*>(
            mmap(nullptr, sizeof(SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0));
        if (shm_ == MAP_FAILED) throw std::runtime_error("mmap failed");

        shm_->version = PROTOCOL_VERSION;
        shm_->slot_tail.store(0);
        shm_->slot_head.store(0);
        shm_->data_tail.store(0);
        for (uint32_t i = 0; i < SLOT_COUNT; ++i)
            shm_->slots[i].ready.store(0);
    }

    ~ProducerNode() {
        if (shm_ != MAP_FAILED) munmap(shm_, sizeof(SharedMemory));
        if (fd_ != -1) close(fd_);
        shm_unlink(name_);
    }

    void Send(uint32_t type, const void* data, uint32_t length) {
        uint32_t idx = shm_->slot_tail.fetch_add(1, std::memory_order_relaxed) % SLOT_COUNT;
        Slot& slot = shm_->slots[idx];

        while (slot.ready.load(std::memory_order_acquire) != 0) {}

        uint32_t doff = shm_->data_tail.fetch_add(length, std::memory_order_relaxed);
        WriteData(shm_->data_buffer, doff, data, length);

        slot.type        = type;
        slot.length      = length;
        slot.data_offset = doff;
        slot.ready.store(1, std::memory_order_release);
    }

private:
    int fd_ = -1;
    SharedMemory* shm_ = static_cast<SharedMemory*>(MAP_FAILED);
    const char* name_;
};

class ConsumerNode {
public:
    ConsumerNode(const char* name) {
        fd_ = shm_open(name, O_RDWR, 0666);
        if (fd_ == -1) throw std::runtime_error("shm_open failed");
        shm_ = static_cast<SharedMemory*>(
            mmap(nullptr, sizeof(SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0));
        if (shm_ == MAP_FAILED) throw std::runtime_error("mmap failed");
        if (shm_->version != PROTOCOL_VERSION)
            throw std::runtime_error("Protocol version mismatch");
    }

    ~ConsumerNode() {
        if (shm_ != MAP_FAILED) munmap(shm_, sizeof(SharedMemory));
        if (fd_ != -1) close(fd_);
    }

    std::optional<Message> Recv(int filter_type = -1) {
        while (true) {
            uint32_t idx = shm_->slot_head.load(std::memory_order_relaxed) % SLOT_COUNT;
            Slot& slot = shm_->slots[idx];

            if (slot.ready.load(std::memory_order_acquire) == 0)
                return std::nullopt;

            Message msg;
            msg.header.type   = slot.type;
            msg.header.length = slot.length;
            msg.payload.resize(slot.length);
            ReadData(shm_->data_buffer, slot.data_offset, msg.payload.data(), slot.length);

            slot.ready.store(0, std::memory_order_release);
            shm_->slot_head.fetch_add(1, std::memory_order_relaxed);

            if (filter_type == -1 || static_cast<int>(msg.header.type) == filter_type)
                return msg;
        }
    }

private:
    int fd_ = -1;
    SharedMemory* shm_ = static_cast<SharedMemory*>(MAP_FAILED);
};
