To run baliset automatically on Bela startup, make sure the contents of this repo are under /root/baliset and that the project has been compiled.
Put baliset.service under /lib/systemd/system/baliset.service and run
```
systemctl enable baliset
```
On reboot, the baliset audio server and ui server should start in a screen session.
(or run `systemctl start baliset` to run it now)
