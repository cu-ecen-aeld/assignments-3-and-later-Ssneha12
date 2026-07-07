# Kernel Crash Analysis – `/dev/faulty`

## Issue Description

When writing data to the device:

```bash
echo "hello_world" > /dev/faulty
```

the Linux kernel encounters a **kernel panic (OOPS)** caused by a **NULL pointer dereference** inside the driver's `faulty_write()` function.

---

# Error Summary

```
Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
Internal error: Oops: 0000000096000045 [#1] SMP
```

### Key Information

| Field | Value |
|-------|-------|
| Fault Type | Data Abort (DABT) |
| Exception Level | Current EL |
| Fault Status Code | Level 1 Translation Fault |
| Access Type | Write (WnR = 1) |
| Crash Location | `faulty_write+0x10/0x20 [faulty]` |

---

# Register Dump

```
x0 : 0000000000000000
x1 : 0000000000000000
x2 : 000000000000000c
x3 : ffffffc008ddbdc0

pc : faulty_write+0x10/0x20 [faulty]
lr : vfs_write+0xc8/0x390
```

### Important Observation

- `x0 = 0x0`
- `x0` is being used as a pointer.
- The kernel attempts to write through this pointer.
- Since the pointer is **NULL**, a kernel OOPS occurs.

---

# Call Trace

```
faulty_write+0x10/0x20 [faulty]
ksys_write+0x74/0x110
__arm64_sys_write+0x1c/0x30
invoke_syscall+0x54/0x130
el0_svc_common.constprop.0+0x44/0xf0
do_el0_svc+0x2c/0xc0
el0_svc+0x2c/0x90
el0t_64_sync_handler+0xf4/0x120
el0t_64_sync+0x18c/0x190
```

---

# Root Cause Analysis

The crash occurs because `faulty_write()` dereferences a **NULL pointer**.

Example of the problematic code:

```c
char *ptr = NULL;

*ptr = 'A';      // NULL pointer dereference
```

Since `ptr` is `NULL`, writing to it causes a **Data Abort**, leading to a kernel OOPS.

---

# Why the Kernel Crashes

During the write operation:

1. User executes:

   ```bash
   echo "hello_world" > /dev/faulty
   ```

2. The kernel invokes the driver's `faulty_write()` callback.

3. `faulty_write()` attempts to write through a pointer whose value is `NULL`.

4. The ARM64 MMU detects an invalid memory access.

5. A **Level 1 Translation Fault** is generated.

6. The kernel reports an OOPS and terminates the offending process.

---

# Recommended Fixes

## 1. Allocate Memory Before Use

Instead of:

```c
char *ptr = NULL;
```

allocate memory:

```c
char *ptr;

ptr = kmalloc(10, GFP_KERNEL);
if (!ptr)
    return -ENOMEM;
```

---

## 2. Validate Pointers

Always verify pointers before dereferencing them.

```c
if (!ptr)
    return -EINVAL;
```

---

## 3. Safely Copy Data from User Space

Never access user-space memory directly.

Use:

```c
if (copy_from_user(buffer, user_buf, count))
    return -EFAULT;
```

instead of directly dereferencing user pointers.

---

# Loaded Kernel Modules

```
hello(O)
faulty(O)
scull(O)
```

---

# Crash Summary

| Item | Description |
|------|-------------|
| Device | `/dev/faulty` |
| Operation | Write |
| Function | `faulty_write()` |
| Fault | NULL Pointer Dereference |
| Register | `x0 = 0x0` |
| Exception | Data Abort |
| Result | Kernel OOPS |

---

# Conclusion

Writing to `/dev/faulty` triggers a **NULL pointer dereference** inside the driver's `faulty_write()` function. The kernel attempts to access memory at address `0x0`, resulting in a **Data Abort** and a kernel OOPS.

The issue can be resolved by:

- Allocating memory before use (`kmalloc()`).
- Validating pointers before dereferencing.
- Using `copy_from_user()` for safe handling of user-space buffers.
- Returning appropriate error codes (`-ENOMEM`, `-EINVAL`, `-EFAULT`) when failures occur.

Implementing these practices prevents invalid memory accesses and improves the stability and reliability of the kernel module.
