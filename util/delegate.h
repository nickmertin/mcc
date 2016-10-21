#ifndef ISU_DELEGATE_H
#define ISU_DELEGATE_H

struct delegate_t {
    void (*func)(void *, void *);
    void *state;
};

struct delegate_t delegate_wrap(void (*func)(void *));

void delegate_invoke(struct delegate_t delegate, void *data);

#endif //ISU_DELEGATE_H
