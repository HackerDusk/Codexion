/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/06 02:48:47 by srandro           #+#    #+#             */
/*   Updated: 2026/09/08 00:33:36 by srandro          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <stdlib.h>
#include <time.h>

int	heap_initializer(t_monitor *monitor)
{
	monitor->heap = malloc(sizeof(t_heap));
	if (!monitor->heap)
		return (0);
	monitor->heap->arr = (
			malloc(sizeof(t_coder *) * (monitor->nb_coders)));
	if (!monitor->heap->arr)
	{
		free(monitor->heap);
		monitor->heap = NULL;
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

void	heap_push(t_heap *heap, t_coder *coder)
{
	t_coder	*tmp;
	size_t	i;
	size_t	parent;

	if (heap->size >= heap->capacity)
		return ;
	heap->arr[heap->size] = coder;
	i = heap->size;
	heap->size++;
	while (i > 0)
	{
		parent = (i - 1) / 2;
		if (heap->cmp(heap->arr[i], heap->arr[parent]))
		{
			tmp = heap->arr[i];
			heap->arr[i] = heap->arr[parent];
			heap->arr[parent] = tmp;
			i = parent;
		}
		else
			break ;
	}
}

void	swap_coders(t_heap *heap, size_t smallest, size_t i)
{
	t_coder	*tmp;

	tmp = heap->arr[smallest];
	heap->arr[smallest] = heap->arr[i];
	heap->arr[i] = tmp;
	i = smallest;
}

t_coder	*heap_pop(t_heap *heap)
{
	t_coder	*res;
	size_t	second_child;
	size_t	i;
	size_t	smallest;

	if (heap->size == 0)
		return (NULL);
	res = heap->arr[0];
	heap->arr[0] = heap->arr[heap->size - 1];
	heap->size--;
	i = 0;
	while (2 * i + 1 < heap->size)
	{
		second_child = 2 * i + 2;
		smallest = 2 * i + 1;
		if (second_child < heap->size
			&& heap->cmp(heap->arr[second_child], heap->arr[smallest]))
			smallest = second_child;
		if (heap->cmp(heap->arr[smallest], heap->arr[i]))
			swap_coders(heap, smallest, i);
		else
			break ;
	}
	return (res);
}
