#include "ring_buffer.h"
#include <string.h>

bool rb_is_empty(const ring_buffer_t *rb)
{
    return rb->read_index == rb->write_index;
}

bool rb_is_full(const ring_buffer_t *rb)
{
    return ((rb->write_index + 1) % RING_BUFFER_SIZE) == rb->read_index;
}

uint16_t rb_get_available(const ring_buffer_t *rb)
{
    return (rb->write_index - rb->read_index) % RING_BUFFER_SIZE;
}

uint16_t rb_get_free_space(const ring_buffer_t *rb)
{
    return RING_BUFFER_SIZE - rb_get_available(rb) - 1;
}

bool rb_put_byte(ring_buffer_t *rb, uint8_t byte)
{
    if (rb_is_full(rb))
        return false;

    rb->data[rb->write_index] = byte;
    rb->write_index           = (rb->write_index + 1) % RING_BUFFER_SIZE;
    return true;
}

uint16_t rb_put_bytes(ring_buffer_t *rb, const uint8_t *data, uint16_t len)
{
    uint16_t free = rb_get_free_space(rb);
    if (len > free)
        len = free;
    if (len == 0)
        return 0;

    uint16_t start      = rb->write_index;
    uint16_t contiguous = RING_BUFFER_SIZE - start;

    if (len <= contiguous) {
        memcpy(&rb->data[start], data, len);
    } else {
        memcpy(&rb->data[start], data, contiguous);
        memcpy(rb->data, data + contiguous, len - contiguous);
    }

    rb->write_index = (start + len) % RING_BUFFER_SIZE;
    return len;
}

bool rb_get_byte(ring_buffer_t *rb, uint8_t *byte)
{
    if (rb_is_empty(rb))
        return false;

    *byte          = rb->data[rb->read_index];
    rb->read_index = (rb->read_index + 1) % RING_BUFFER_SIZE;
    return true;
}

uint16_t rb_get_bytes(ring_buffer_t *rb, uint8_t *data, uint16_t len)
{
    uint16_t avail = rb_get_available(rb);
    if (len > avail)
        len = avail;
    if (len == 0)
        return 0;

    uint16_t start      = rb->read_index;
    uint16_t contiguous = RING_BUFFER_SIZE - start;

    if (len <= contiguous) {
        memcpy(data, &rb->data[start], len);
    } else {
        memcpy(data, &rb->data[start], contiguous);
        memcpy(data + contiguous, rb->data, len - contiguous);
    }

    rb->read_index = (start + len) % RING_BUFFER_SIZE;
    return len;
}

bool rb_peek_byte(const ring_buffer_t *rb, uint16_t offset, uint8_t *byte)
{
    uint16_t avail = rb_get_available(rb);
    if (offset >= avail)
        return false;

    uint16_t index = (rb->read_index + offset) % RING_BUFFER_SIZE;
    *byte          = rb->data[index];
    return true;
}

uint16_t rb_peek_block(const ring_buffer_t *rb, uint16_t offset, uint8_t *dest, uint16_t len)
{
    uint16_t avail = rb_get_available(rb);
    if (offset >= avail)
        return 0;
    if (len > avail - offset)
        len = avail - offset;

    uint16_t start      = (rb->read_index + offset) % RING_BUFFER_SIZE;
    uint16_t contiguous = RING_BUFFER_SIZE - start;

    if (len <= contiguous) {
        memcpy(dest, &rb->data[start], len);
    } else {
        memcpy(dest, &rb->data[start], contiguous);
        memcpy(dest + contiguous, rb->data, len - contiguous);
    }
    return len;
}

uint16_t rb_get_contiguous_length(const ring_buffer_t *rb, uint16_t offset)
{
    uint16_t avail = rb_get_available(rb);
    if (offset >= avail)
        return 0;

    uint16_t start      = (rb->read_index + offset) % RING_BUFFER_SIZE;
    uint16_t contiguous = RING_BUFFER_SIZE - start;
    uint16_t remaining  = avail - offset;

    return (contiguous < remaining) ? contiguous : remaining;
}

uint16_t rb_skip_bytes(ring_buffer_t *rb, uint16_t len)
{
    uint16_t avail = rb_get_available(rb);
    if (len > avail)
        len = avail;

    rb->read_index = (rb->read_index + len) % RING_BUFFER_SIZE;
    return len;
}
