task :import_likes => :environment do
   Admin.import_facebook_likes
end

task :clean_causes => :environment do
  Admin.clean_causes
end 