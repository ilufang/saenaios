Issue List
==========

Interrupt handler does not save registers
-----------------------------------------

**Issue #4 Opened on Mar 25, 2018 8:00pm by Fang Lu (@fanglu2)**

> Registers are clobbered after interrupt handler execution

1 Related Merge Request

* `!11` Resolve "Interrupt handler does not save registers"
  * `f2e9f6d2`: Save registers for IRQ handlers

Activity

* Fang Lu (@fanglu2) created branch
  `6-interrupt-handler-does-not-save-registers` on Mar 25, 2018 8:00pm
* Fang Lu (@fanglu2) mentioned in merge request `!11` on Mar 25, 2018 8:00pm
* Fang Lu (@fanglu2) closed via commit `7f5d0d26` on Mar 25, 2018 10:20pm
* Fang Lu (@fanglu2) closed via merge request `!11` on Mar 25, 2018 10:20pm

Control-L behavior in stdin buffer
----------------------------------

**Issue #8 Opened on Mar 27, 2018 4:06pm by Fang Lu (@fanglu2)**

> CP2 specification might not expect '^L' character being read from stdin stream

1 Related Merge Request

* `!19` Resolve "Control-L behavior in stdin buffer"
  * `dbce3cf2`: #8: temp workaround for CP2 demo

Activity

* Fang Lu @fanglu2 created branch `8-control-l-behavior-in-stdin-buffer` on
  on Mar 27, 2018 4:06pm
* Fang Lu (@fanglu2) mentioned in merge request `!19` on Mar 27, 2018 4:06pm
* Fang Lu (@fanglu2) mentioned in commit `dbce3cf2` on Mar 27, 2018 4:22pm
* Fang Lu (@fanglu2) mentioned in commit `d7a0ba0c` on Mar 27, 2018 4:55pm
* Fang Lu (@fanglu2) closed via merge request `!19` on Mar 27, 2018 4:55pm

CP-3 Issues
-----------

**Issue #14 Opened 3 weeks ago by Fang Lu (@fanglu2)**

> * [x] *test_execute* does not return and make other tests unreachable
> * [x] *test_paging* fails on TLB tests
> * [x] devfs multiple *get_sb* calls clear device table
> * [x] vfs posix test: *readdir* fails on /dev
> * [x] fs_tests has fd leaks
> * [x] executing shell crashes system

1 Related Merge Request

* `!22` Resolve "cp3 issues"
  * `f9c342f1`: Fix DIRENT_INDEX_AUTO listing index translation
  * `b166dc6b`: fix failure on tlb test, wrongly flush test removed
  * `e8b080cd`: Resolve compiler warnings
  * `e1d1c4f6`: Fix #14 item 4
  * `af9fb59e`: Fix #14 item 5, which fixes item 6.
  * `fbe726c1`: Fix #14 item 1, 3

Activity

* Fang Lu (@fanglu2) commented on Apr 9, 2018 1:12pm
  * @fanglu2 @xutaoj2
* Fang Lu (@fanglu2) created branch `14-cp3-issues` on Apr 9, 2018 1:12pm
* Fang Lu (@fanglu2) mentioned in merge request `!22` on Apr 9, 2018 1:12pm
* Fang Lu (@fanglu2) commented on Apr 9, 2018 1:13pm
  * Please use branch `14-cp3-issues`. Merge in to master only when and as
    soon as fully tested.
* Fang Lu (@fanglu2) mentioned in commit `fbe726c1` on Apr 9, 2018 1:15pm
* Fang Lu (@fanglu2) marked the task **`test_execute` does not return and make
  other tests unreachable** as completed on Apr 9, 2018 1:16pm
* Fang Lu (@fanglu2) marked the task **devfs multiple `get_sb` calls clear
  device table** as completed on Apr 9, 2018 1:16pm
* Fang Lu (@fanglu2) mentioned in commit `af9fb59e` on Apr 9, 2018 1:33pm
* Fang Lu (@fanglu2) marked the task **`fs_tests` has fd leaks** as completed on
  Apr 9, 2018 1:33pm
* Fang Lu (@fanglu2) marked the task **executing shell crashes system** as
  completed on Apr 9, 2018 1:33pm
* Fang Lu (@fanglu2) mentioned in commit `e1d1c4f6` on Apr 9, 2018 2:25pm
* Fang Lu (@fanglu2) marked the task **vfs posix test: readdir fails on /dev**
  as completed on Apr 9, 2018 2:25pm
* Fang Lu (@fanglu2) commented on Apr 9, 2018 2:26pm
  * Disabling interactive mp3fs tests: these tests should be converted to
    automated tests instead. @xutaoj2
* xutaoj2 (@xutaoj2) marked the task **test_paging fails on TLB tests** as
  completed on Apr 9, 2018 3:59pm
* Fang Lu (@fanglu2) closed via commit `af9fb59e` on Apr 9, 2018 4:53pm
* Fang Lu (@fanglu2) closed via commit `fbe726c1` on Apr 9, 2018 4:53pm
* Fang Lu (@fanglu2) closed via commit `e1d1c4f6` on Apr 9, 2018 4:53pm
* Fang Lu (@fanglu2) mentioned in commit `8eadce91` on Apr 9, 2018 4:53pm
* Fang Lu (@fanglu2) closed via merge request `!22` on Apr 9, 2018 4:53pm

RTC in MP3FS not being routed
-----------------------------

**Issue #15 Opened on Apr 12, 2018 2:27am by Fang Lu (@fanglu2)**

> `pingpong` might not be working as expected due to RTC IO not routed to the
> actual RTC driver.
>
> The actual RTC driver is attached to the devfs mounted at /dev, NOT the rtc
> file in MP3FS. MP3FS is expected to transparently convert any file with type 0
> (RTC device file type) into the actual devfs RTC device by returning a symlink
> pointing to `/dev/rtc`.

1 Related Merge Request

* `!25` Resolve "RTC in MP3FS not being routed"
  * `9b944675`: rtc fixed, brutal magic!

Activity

* xutaoj2 (@xutaoj2) created branch `15-rtc-in-mp3fs-not-being-routed` on Apr
  14, 2018 5:54pm
* xutaoj2 (@xutaoj2) mentioned in merge request `!25` on Apr 14, 2018 7:02pm
* xutaoj2 (@xutaoj2) closed via merge request `!25` on Apr 14, 2018 7:03pm
* xutaoj2 (@xutaoj2) mentioned in commit `7d60fa66` on Apr 14, 2018 7:03pm

.file pathname process dead loop
--------------------------------

**Opened on Apr 15, 2018 10:27pm by xutaoj2 (@xutaoj2)**


1 Related Merge Request

* `!26` Resolve ".file pathname process dead loop"
  * `4e8578c1`: .file pathname process dead loop fix

Activity

* xutaoj2 (@xutaoj2) created branch `17-file-pathname-process-dead-loop` on Apr
  15, 2018 10:27pm
* xutaoj2 (@xutaoj2) mentioned in merge request `!26` on Apr 15, 2018 10:31pm
* xutaoj2 (@xutaoj2) mentioned in commit `e3b00722` on Apr 15, 2018 10:31pm
* xutaoj2 (@xutaoj2) closed via merge request `!26` on Apr 15, 2018 10:31pm

Process Memory Leak
-------------------

**Opened on Apr 27, 2018 10:51pm by xutaoj2 (@xutaoj2)**

> Unable to run more process after around 60 ls because of ENOMEM

1 Related Merge Request

* `!29` Resolve "Process Memory Leak"
  * `896be829`: physical memory allocation leak fixed

Activity

* xutaoj2 (@xutaoj2) created branch `21-process-memory-leak` on Apr 27, 2018
  10:51pm
* xutaoj2 (@xutaoj2) mentioned in merge request `!29` on Apr 27, 2018 11:43pm
* xutaoj2 (@xutaoj2) closed via merge request `!29` on Apr 27, 2018 11:43pm
* xutaoj2 (@xutaoj2) mentioned in commit `a1844b3d` Apr 27, 2018 11:43pm
