#pragma once

#include <core/types.h>
#include <core/common.h>
#include <core/allocator.h>

#define MALLOC(x) malloc(x)
#define FREE(x) free(x)

template <typename T>
struct Array {
    Array(T* _data = 0, int64 _count = 0) : data(_data), count(_count) {}

    template <size_t N>
    Array(const T (&c_arr)[N]) : data(c_arr), count(N) {}

    Array sub_array(int64 _offset, int64 _count = -1) {
        ASSERT(0 <= _offset);
		ASSERT(_count > 0 || _count == -1);
        if (_count == -1) {
            _count = (this->count - _offset) > 0 ? (this->count - _offset) : 0;
        }
		ASSERT((_offset + _count) <= this->count);
        return {data + _offset, _count};
    }

    const T* begin() const { return data; }
    const T* beg() const { return data; }
    const T* end() const { return data + count; }

    T* begin() { return data; }
    T* beg() { return data; }
    T* end() { return data + count; }

    const T& front() const { return data[0]; }
    const T& back() const { return data[count - 1]; }
    T& front() { return data[0]; }
    T& back() { return data[count - 1]; }

    operator bool() const { return data != nullptr && count > 0; }
    const T& operator[](int64 i) const { return data[i]; }
    T& operator[](int64 i) { return data[i]; }

    T* data;
    int64 count;
};

// Light-weight std::vector alternative
template <typename T>
struct DynamicArray : Array<T> {
	static constexpr int64 INIT_CAPACITY = 32;
    DynamicArray() : capacity(INIT_CAPACITY) {
		this->data = (T*)MALLOC(capacity * sizeof(T));
        this->count = 0;
    }

	DynamicArray(int64 count, T value = 0) : capacity(count > INIT_CAPACITY ? count : INIT_CAPACITY) {
		this->data = (T*)MALLOC(capacity * sizeof(T));
		this->count = count;
		memset(this->data, (int)value, this->count * sizeof(T));
	}

    DynamicArray(const Array<T>& clone_source) : capacity(clone_source.count) {
        this->count = capacity;
        if (this->count > 0) {
            this->data = (T*)MALLOC(capacity * sizeof(T));
            memcpy(this->data, clone_source.data, this->count * sizeof(T));
        }
    }

	DynamicArray(const DynamicArray& other) : capacity(other.capacity) {
		this->data = (T*)MALLOC(capacity * sizeof(T));
		this->count = other.count;
		memcpy(this->data, other.data, this->count * sizeof(T));
	}

	DynamicArray(DynamicArray&& other) : capacity(other.capacity) {
		this->data = other.data;
		other.data = nullptr;
		this->count = other.count;
		other.count = 0;
	}

    ~DynamicArray() {
        if (this->data) {
			FREE(this->data);
        }
		this->count = 0;
    }

	DynamicArray& operator =(const DynamicArray& other) {
		if (&other != this) {
			if (other.count > capacity) {
				reserve(other.count);
			}
			this->count = other.count;
			memcpy(this->data, other.data, this->count * sizeof(T));
		}
		return *this;
	}

	DynamicArray& operator =(DynamicArray&& other) {
		// Is this check needed?
		if (&other != this) {
			if (this->data) {
				FREE(this->data);
			}
			capacity = other.capacity;
			other.capacity = 0;
			this->data = other.data;
			other.data = nullptr;
			this->count = other.count;
			other.count = 0;
		}
		return *this;
	}

	int64 size() const { return this->count; }

    void push_back(const T& item) {
        if (this->count >= capacity) {
            // GROW
			auto new_capacity = capacity < INIT_CAPACITY ? INIT_CAPACITY : capacity * 2;
            reserve(new_capacity);
        }
        this->data[this->count] = item;
        this->count++;
    }

    void reserve(int64 new_capacity) {
        if (new_capacity < capacity) return;
		T* new_data = (T*)MALLOC(new_capacity * sizeof(T));
        if (this->data) {
            memcpy(new_data, this->data, this->count * sizeof(T));
        }
		FREE(this->data);
        this->data = new_data;
        capacity = new_capacity;
    }

    // Resizes the array to a new size and zeros eventual new slots
    void resize(int64 new_count) {
        ASSERT(new_count > 0);
        if (new_count == this->count)
            return;
        else if (new_count < this->count) {
            this->count = new_count;
            return;
        } else {
            if (capacity < new_count) {
                reserve(new_count);
                this->count = new_count;
            }
        }
    }

    void clear() {
        this->count = 0;
    }

private:
	/*
	T* internal_alloc(size_t size) {
		if (allocator)
			return (T*)allocator->alloc(size * sizeof(T));
		else {
			T* ptr = (T*)MALLOC(size * sizeof(T));
			printf("Allocating %u into ptr %p at [%s][%i]\n", size * sizeof(T), ptr, __FUNCTION__, __LINE__);
			return ptr;
		}
	}

	void internal_free(T* mem) {
		if (allocator)
			allocator->free(mem);
		else {
			printf("Freeing ptr %p at [%s][%i]\n", mem, __FUNCTION__, __LINE__);
			FREE(mem);
		}
	}
	*/

    int64 capacity;
    //Allocator* allocator = nullptr;
};

struct CString : Array<const char> {
	CString() {
		data = 0;
		count = 0;
	}

	CString(const char* cstr, int64 length = -1) {
		data = cstr;
		if (length == -1)
			length = strlen(cstr);
		count = length;
	}

	template <int64 length>
	CString(const char(&cstr)[length]) {
		data = cstr;
		count = length;
	}

	CString substr(int64 _offset, int64 _count = -1) {
		auto array = sub_array(_offset, _count);
		return { array.data, array.count };
	}

	operator const char*() { return data; }
	operator bool() { return (data != 0 && count != 0); }
};

struct String : Array<char> {
    String() {
        data = 0;
        count = 0;
    }

    String(const String& other) : Array(other.data, other.count) {}

    String(char* cstr, int64 length) {
        data = cstr;
        count = length;
    }

    template <int64 length>
    String(char (&cstr)[length]) {
        data = cstr;
        count = length;
    }

    String substr(int64 _offset, int64 _count = -1) {
        auto array = sub_array(_offset, _count);
        return {array.data, array.count};
    }

    operator CString() { return CString(data, count); }
    operator const char*() { return data; }
    operator bool() { return (data != 0 && count != 0); }
};



template <typename T>
Array<T> allocate_array(int64 count, Allocator& alloc = default_alloc) noexcept {
    ASSERT(count > 0);
    return {(T*)alloc.alloc(sizeof(T) * count), count};
}

template <typename T>
Array<T> allocate_array_and_zero(int64 count, Allocator& alloc = default_alloc) noexcept {
    ASSERT(count > 0);
    Array<T> array = allocate_array<T>(count, alloc);
    memset(array.data, 0, array.count * sizeof(T));
    return array;
}

