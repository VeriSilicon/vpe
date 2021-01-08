Srmtool is tool to do below two things:

## 1. Resource Monitoring:

It will show the HW status include:
* Encoder usage(percentage)
* Decoder usage(percentage)
* Memory used (MB)
* Mempry free (MB)

###### Command line example:
```bash
./srmtool

[vsi@vsi ~/zhanggy/vpe/tools]$ ./srmtool
transcoder 0: power=on, decoder=  0%, encoder=  0%, memory used=3452MB, memory free=   0MB - active
transcoder 1: power=on, decoder= 76%, encoder= 80%, memory used= 146MB, memory free=3306MB - active
transcoder 2: power=on, decoder= 45%, encoder= 41%, memory used= 146MB, memory free=3306MB - active
transcoder 3: power=on, decoder= 76%, encoder= 83%, memory used= 293MB, memory free=3159MB - active
transcoder 4: power=on, decoder= 79%, encoder= 80%, memory used= 293MB, memory free=3159MB - active
transcoder 5: power=on, decoder= 55%, encoder= 50%, memory used= 293MB, memory free=3159MB - active
transcoder 6: power=on, decoder= 78%, encoder= 81%, memory used= 293MB, memory free=3159MB - active
transcoder 7: power=on, decoder= 79%, encoder= 81%, memory used= 293MB, memory free=3159MB - active
transcoder 8: power=on, decoder=  3%, encoder=  4%, memory used=   0MB, memory free=3454MB -  freee
transcoder 9: power=on, decoder= 79%, encoder= 82%, memory used= 293MB, memory free=3159MB - active

```

## 2. Resource Matcher:

For new task, it can be used to find the most-match transcoder devices.
It can calculate the codec workload on all of the devices, and try to find the most-matched devices.

It support two modes to allocate the resources:

#### 1.Exclusive Mode:

In this mode, once any task is running on the HW, then it can't be assigned to new task any more until it's free.

###### Command line example:
```bash
./srmtool allocate
transcoder7
```

#### 2.Shared Mode:

One HW can be assigned many tasks until reach it's limit.

* If the new task required resources(decoder, encoder, memory) are less than what HW left, then the new task can be assigned to this HW; This mode is called "powersaving mode".

* But to achieve best performance, maybe assign the new task to a complete free HW is the better choise. This mode is called "performance mode".

How to describe the new task required resources? in this program I used resolution:
480p/720p/1080p/2160p.

How many decoder/encoder/memory are required by 480p@30Hz/720p@30/1080p@30/2160p@30 transcoding task,
then 480p/720p/1080p/2160p means how many decoder/encoder/memory resources.

###### Command line example:
```bash
./srmtool allocate 1080p 1 performance
transcoder5
./srmtool allocate 480p 2 powersaving
transcoder0
```