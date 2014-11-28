require 'rubygems'
require 'rake'
require 'fileutils'
require "bundler"

desc "CI: Build stage"
task :build do
  RAILS_ENV = 'staging'
  Rake::Task['db:migrate'].invoke
  sh "wheneverize ."
  #sh "bundle exec rake db:migrate"
  Rake::Task['db:test:prepare'].invoke
  RAILS_ENV = 'test'
  sh "bundle install"
  Bundler.setup(:default, :test)

  Rake::Task['db:create'].invoke
  Rake::Task['db:migrate'].invoke
  Rake::Task['spec'].prerequisites.clear
  Rake::Task['spec'].invoke
  
 # Rake::Task['cucumber:ok'].prerequisites.clear
 # Rake::Task['cucumber:ok'].invoke
  sh "whenever --update-crontab portoalegrecc --set environment=staging"
end