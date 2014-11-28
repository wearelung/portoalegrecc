job_type :awesome_rake,    "BUNDLE_GEMFILE=/home/ubuntu/www/portoalegrecc/current/Gemfile RAILS_ENV=:environment /home/ubuntu/.rvm/gems/ruby-1.8.7-p302@global/bin/bundle exec /home/ubuntu/.rvm/gems/ruby-1.8.7-p302/bin/rake -f /home/ubuntu/www/portoalegrecc/current/Rakefile :task "
job_type :production_rake,    "BUNDLE_GEMFILE=/home/ubuntu/www/portoalegrecc/Gemfile RAILS_ENV=:environment /home/ubuntu/.rvm/gems/ruby-1.8.7-p352/bin/bundle exec /home/ubuntu/.rvm/gems/ruby-1.8.7-p352/bin/rake -f /home/ubuntu/www/portoalegrecc/Rakefile :task "

#RAILS_ENV=staging /home/ubuntu/.rvm/gems/ruby-1.8.7-p302/bin/rake -f /home/ubuntu/www/portoalegrecc/current/Rakefile  import_likes 

#job_type :awesome_command,    ". /home/egpotter/.profile && :task"

# Use this file to easily define all of your cron jobs.
#
# It's helpful, but not entirely necessary to understand cron before proceeding.
# http://en.wikipedia.org/wiki/Cron

# Example:
#
# set :output, "/path/to/my/cron_log.log"
#
# every 2.hours do
#   command "/usr/bin/some_great_command"
#   runner "MyModel.some_method"
#   rake "some:great:rake:task"
# end
#
# every 4.days do
#   runner "AnotherModel.prune_old_records"
# end

#every 3.minutes do 
#  rake 'import_likes'
#end
every 30.minutes do 
  awesome_rake 'import_likes', :environment => :staging
  production_rake 'import_likes', :environment => :production
#  rake 'import_likes', :environment => :staging
#  awesome_command "env > /home/egpotter/workspace/portoalegrecc/awesome_cron_env"
#  command "env > /home/egpotter/workspace/portoalegrecc/cron_env"
end
every 4.hours do 
  awesome_rake 'clean_causes', :environment => :staging
  production_rake 'clean_causes', :environment => :production
end


# Learn more: http://github.com/javan/whenever
