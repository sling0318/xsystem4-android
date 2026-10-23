#include "vm.h"

#ifdef __ANDROID__
#include <jni.h>

JNIEXPORT void JNICALL
Java_io_github_kichikuou_xsystem4_XSystem4Activity_nativeQueueGameAction(
	JNIEnv *env, jobject thiz, jint action, jint arg)
{
	vm_queue_game_action_arg((enum vm_game_action)action, arg);
}
#endif
