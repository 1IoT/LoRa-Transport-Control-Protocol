#pragma once
#include <Arduino.h>

template <typename T>
struct LinkedListItem
{
    T *item;
    LinkedListItem<T> *next;
    LinkedListItem<T> *before;
};

template <typename T>
class LinkedList
{
public:
    void addFirst(T *item)
    {
        LinkedListItem<T> *newLinkedListItem = new LinkedListItem<T>();
        newLinkedListItem->item = item;
        newLinkedListItem->next = firstItem;
        newLinkedListItem->before = nullptr;
        length++;

        if (firstItem)
        {
            firstItem->before = newLinkedListItem;
            firstItem = newLinkedListItem;
        }
        else
        {
            firstItem = newLinkedListItem;
            lastItem = newLinkedListItem;
        }
    }

    void deleteFirst()
    {
        if (firstItem)
        {
            LinkedListItem<T> *tempLinkedListItem = firstItem;

            if (firstItem->before)
            {
                firstItem = firstItem->before;
                firstItem->next = nullptr;
            }
            else
            {
                firstItem = nullptr;
                lastItem = nullptr;
            }

            delete tempLinkedListItem->item;
            delete tempLinkedListItem;
            length--;
        }
    }

    void addLast(T *item)
    {
        LinkedListItem<T> *newLinkedListItem = new LinkedListItem<T>();
        newLinkedListItem->item = item;
        newLinkedListItem->next = nullptr;
        newLinkedListItem->before = lastItem;
        length++;

        if (lastItem)
        {
            lastItem->next = newLinkedListItem;
            lastItem = newLinkedListItem;
        }
        else
        {
            firstItem = newLinkedListItem;
            lastItem = newLinkedListItem;
        }
    }

    void deleteLast()
    {
        if (lastItem)
        {
            LinkedListItem<T> *tempLinkedListItem = lastItem;

            if (lastItem->next)
            {
                lastItem = lastItem->next;
                lastItem->before = nullptr;
            }
            else
            {
                firstItem = nullptr;
                lastItem = nullptr;
            }

            delete tempLinkedListItem->item;
            delete tempLinkedListItem;
            length--;
        }
    }

    const LinkedListItem<T> *getFirst()
    {
        return firstItem;
    }

    const LinkedListItem<T> *getLast()
    {
        return lastItem;
    }

    int getLength()
    {
        return length;
    }

private:
    LinkedListItem<T> *firstItem = nullptr;
    LinkedListItem<T> *lastItem = nullptr;

    int length = 0;
};