```sh
sudo insmod my_parameters.ko myvalue=10 myname="alper" arr_myvalues=1,11,21,31

[27758.683612] myvalue: 10  
[27758.683638] cb_myvalue: 0  
[27758.683645] myname: alper  
[27758.683653] arr_myvalues[0]: 1  
[27758.683661] arr_myvalues[1]: 11  
[27758.683669] arr_myvalues[2]: 21   
[27758.683677] arr_myvalues[3]: 31   
[27758.683684] Kernel module inserted successfully  
```

```sh
ls -l /sys/module/my_parameters/parameters/

total 0  
-rw------- 1 root root 4096 Dec 19 15:34 arr_myvalues  
-rw-r--r-- 1 root root 4096 Dec 19 15:34 cb_myvalue  
-rw------- 1 root root 4096 Dec 19 15:34 myname  
-rw------- 1 root root 4096 Dec 19 15:34 myvalue  
```

Also, we can read the values with `sudo cat /sys/module/my_parameters/parameters/arr_myvalues` -> `1,11,21,31`

If we want to change the value of any variable we can use echo via terminal. For example, let's change `myvalue`. We can change value with
`sudo sh -c "echo 99 > /sys/module/my_parameters/parameters/myvalue"`. After that we can read it with the `sudo cat /sys/module/my_parameters/parameters/myvalue` command, we can see that it changed.
But there was no notification that `myvalue` has changed in `dmesg`. Because, we used `module_param(name, type, perm)` when we defined `myvalue`. If we used `module_param_cb(name, ops, arg, perm)` as we defined `cb_myvar`
we would see a notification in `dmesg`.

Now, let's change `cb_myvar` value and see the notification in `dmesg`.

```sh
sudo sh -c "echo 55 > /sys/module/my_parameters/parameters/cb_myvalue" && dmesg | tail -2

[29343.146509] Callback function called
[29343.146532] New cb_myvalue: 55
```
So our callback function got called and we got notification about it.
