import time
Import("env")

# UTC epoch: independent of the build computer's timezone.
env.Append(CPPDEFINES=[("BUILD_EPOCH", int(time.time()))])
