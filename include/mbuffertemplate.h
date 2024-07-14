
#ifndef PREFIX
#error No prefix given!
#endif

#ifndef T
#error No type specified!
#else

#include "mbuffer.h"

#define CCAT2(x, y) x##y
#define CCAT(x, y) CCAT2(x, y)
#define FN(x) CCAT(x, PREFIX)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

// append a member to the array
static int FN(m_append_member_)(struct managedBuffer *buffer, T input)
{
	buffer->size++;
	if (m_realloc(buffer, buffer->size)) {
		return -1;
	}
	((T *)buffer->b)[buffer->size - 1] = input;
	return 0;
}

// append multiple members to the array
static int FN(m_append_array_)(struct managedBuffer *buffer, T *input,
			       size_t size)
{
	buffer->size += size;
	if (m_realloc(buffer, buffer->size * buffer->typesize)) {
		return -1;
	}
	memcpy(&((T *)buffer->b)[buffer->size - size], input,
	       size * buffer->typesize);
	return 0;
}

// cast managedBuffer to the template type
static T *FN(m_)(struct managedBuffer *buffer)
{
	return (T *)(buffer->b);
}

#pragma GCC diagnostic pop

#undef CCAT2
#undef CCAT
#undef FN
#undef T
#undef PREFIX

#endif
