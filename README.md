chinese2pinyin is used for converting Chinese to pinyin.

You can create a Cron Job (Scheduled Task) to keep update your indexing according to your requirements.
It is good way to set up several separate Cron Jobs for different directory base on frequenncy of use.

For example:
You can index your directory every 4 hours by add this to crontab:
* */4 * * * /usr/bin/chinese2pinyin -i /home/username/Downloads

if you are not familar with how to setup your Cron Job. You can use this website to help you.
http://www.corntab.com/pages/crontab-gui

Add travis CI support.

