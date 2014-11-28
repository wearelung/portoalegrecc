require 'rubygems'
require 'rufus/scheduler'

scheduler = Rufus::Scheduler.start_new  
  
scheduler.cron("00 03 * * *") do
  Admin.import_facebook_likes  
end   