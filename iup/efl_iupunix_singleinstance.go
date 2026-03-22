//go:build efl && !windows && !darwin

package iup

/*
#include "external/src/unix/iupunix_singleinstance.c"
*/
import "C"
