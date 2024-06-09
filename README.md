# Recon2024 Demo

Provides commands to read from and write to arbitrary kernel-mode memory for users with the Administrator privilege. HVCI compatible. No test signing mode is required.

We achieve this by installing `DBUtilDrv2.sys` (`71fe5af0f1564dc187eea8d59c0fbc897712afa07d18316d2080330ba17cf009`), which has the IOCTL commands to access arbitrary kernel mode addresses and is not block-listed under HVCI. This is not a security issue as Admin-to-kernel has never been a security boundary.

Code is largely based on https://github.com/worawit/malk.
