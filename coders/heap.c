# include "codexion.h"

int	heap_initializer(t_monitor *monitor)
{
	monitor->heap = malloc(sizeof(t_heap));
	if (!monitor->heap)
		return (0);
	monitor->heap->arr = malloc(sizeof(t_coder *) * (monitor->nb_coders));
	if (!monitor->heap->arr)
	{
		free(monitor->heap);
		return (0);
	}
	monitor->heap->capacity = monitor->nb_coders;
	monitor->heap->size = 0;
	if (!strcmp(monitor->scheduler_type, "edf"))
		monitor->heap->cmp = cmp_edf;
	if (!strcmp(monitor->scheduler_type, "fifo"))
		monitor->heap->cmp = cmp_fifo;
	return (1);
}

void    heap_push(t_heap *heap, t_coder *coder)
{
    size_t  i;
    size_t  parent;
    t_coder *tmp;

    if (heap->size >= heap->capacity)
        return;
    heap->arr[heap->size] = coder;
    i = heap->size;
    heap->size++;
    while (i > 0)
    {
        parent = (i - 1) / 2;
        if (
            heap->cmp(heap->arr[i],
            heap->arr[parent]))
            {
                tmp = heap->arr[i];
                heap->arr[i] = heap->arr[parent];
                heap->arr[parent] = tmp;
                i = parent;
            }
        else
            break;
    }
}

t_coder *heap_pop(t_heap *heap)
{
    t_coder *res;
    t_coder *tmp;
    size_t  left;
    size_t  right;
    size_t  i;
    size_t  smallest;

    if (heap->size == 0)
        return (NULL);
    res = heap->arr[0];
    heap->arr[0] = heap->arr[heap->size - 1];
    heap->size--;
    i = 0;
    while (2 * i + 1 < heap->size)
    {
        left = 2 * i + 1;
        right = 2 * i + 2;
        smallest = left;
        if (right < heap->size && heap->cmp(heap->arr[right], heap->arr[left]))
            smallest = right;
        if (heap->cmp(heap->arr[smallest], heap->arr[i]))
        {
            tmp = heap->arr[smallest];
            heap->arr[smallest] = heap->arr[i];
            heap->arr[i] = tmp;
            i = smallest;
        }
        else
            break;
    }
    return res;
}