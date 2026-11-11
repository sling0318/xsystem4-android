#include "vm.h"

#ifdef __ANDROID__
#include <jni.h>
#include "cheat.h"

JNIEXPORT void JNICALL
Java_io_github_kichikuou_xsystem4_XSystem4Activity_nativeQueueGameAction(
	JNIEnv *env, jobject thiz, jint action)
{
	vm_queue_game_action((enum vm_game_action)action);
}

JNIEXPORT jint JNICALL
Java_io_github_kichikuou_xsystem4_XSystem4Activity_nativeCheatCount(
	JNIEnv *env, jobject thiz)
{
	cheat_init();
	return cheat_entry_count();
}

JNIEXPORT jstring JNICALL
Java_io_github_kichikuou_xsystem4_XSystem4Activity_nativeCheatLabel(
	JNIEnv *env, jobject thiz, jint index)
{
	const char *label = cheat_entry_label(index);
	if (!label)
		return NULL;
	return (*env)->NewStringUTF(env, label);
}

// Returns the current value, or CHEAT_INVALID_VALUE if the addressed variable
// can't be located right now (game not running / mid-load).
#define CHEAT_INVALID_VALUE (-0x8000000000000000LL)

JNIEXPORT jlong JNICALL
Java_io_github_kichikuou_xsystem4_XSystem4Activity_nativeCheatValue(
	JNIEnv *env, jobject thiz, jint index)
{
	int32_t value;
	if (!cheat_entry_get(index, &value))
		return CHEAT_INVALID_VALUE;
	return (jlong)value;
}

JNIEXPORT jboolean JNICALL
Java_io_github_kichikuou_xsystem4_XSystem4Activity_nativeCheatSet(
	JNIEnv *env, jobject thiz, jint index, jint value)
{
	return cheat_entry_set(index, value);
}
#endif
