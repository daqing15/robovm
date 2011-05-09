#include <nullvm.h>

jboolean nvmInitThreads(Env* env) {
    Thread* thread = nvmAllocateMemory(env, sizeof(Thread));
    if (!thread) return FALSE;
    env->currentThread = thread;

    if (hythread_attach(&thread->hyThread) < 0) return FALSE;

    Method* threadConstructor = nvmGetInstanceMethod(env, java_lang_Thread, "<init>", "(J)V");
    if (!threadConstructor) return FALSE;
    Object* threadObj = nvmNewObject(env, java_lang_Thread, threadConstructor, (jlong) thread);
    if (!threadObj) return FALSE;

    thread->threadObj = threadObj;

    return TRUE;
}

jint nvmMonitorEnter(Env* env, Object* obj) {
    // TODO: Protect initialization with global (spin?)lock
    if (!obj->monitor) {
        IDATA result = hythread_monitor_init_with_name(&obj->monitor, 0, NULL);
        if (result < 0) {
            nvmAbort("Failed to initialize monitor: %d", result);
        }
    }
    IDATA result = hythread_monitor_enter_using_threadId(obj->monitor, env->currentThread->hyThread);
    if (result != 0) {
        if (result == HYTHREAD_PRIORITY_INTERRUPTED) {
            // TODO: What?
        }
        nvmAbort("Unexpected value returned by hythread_monitor_enter_using_threadId(): %d", result);
    }

    return 0;
}

jint nvmMonitorExit(Env* env, Object* obj) {
    // TODO: Protect access to monitor with global (spin?)lock
    if (!obj->monitor) {
        nvmThrowIllegalMonitorStateException(env);
        return -1;
    }
    IDATA result = hythread_monitor_exit_using_threadId(obj->monitor, env->currentThread->hyThread);
    if (result != 0) {
        if (result == HYTHREAD_ILLEGAL_MONITOR_STATE) {
            nvmThrowIllegalMonitorStateException(env);
            return -1;
        }
        nvmAbort("Unexpected value returned by hythread_monitor_exit_using_threadId(): %d", result);
    }
    return 0;
}
