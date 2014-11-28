require "ruport"
require "ruport/util"
require "extend_string"
require "rest-client"
include Spawn

class Admin < ActiveRecord::Base
  
  def self.get_causes (where)
    causes = Cause.find(:all, :select => ['causes.id', 'categories.name as category_name', 'title', 'author', 'abstract', 'local', 'district', 'causes.created_at', 'views', 'is_rejected', 'likes', 'protocol'].join(','), 
      :joins => :category, :conditions => where, :order => "created_at")

    unless causes.empty?
      decoder = HTMLEntities.new
      
      causes.map! {|cause|
        cause.abstract = decoder.decode(cause.abstract)

        cause.abstract.gsub!(/<[^<>]*>/, "")
        cause.abstract.gsub!(/\,/, "")
        
        [cause.id, cause.category_name, cause.title, cause.author, cause.abstract, cause.local, cause.district, cause.created_at, cause.views, cause.is_rejected, cause.likes, cause.protocol]
      }

      causes = Ruport::Data::Table.new :data => causes,
        :column_names => %w[causa_id categoria titulo autor resumo local bairro quando visualizacoes rejeitada likes protocolo_falaPoa]

      causes.save_as("public/reports/causes.csv")
    end
    causes
  end
  
  def self.get_rejected_causes(where)
    causes = Cause.report_table_by_sql("
      SELECT causes.id as causa_id , cat.name as categoria, causes.title as titulo, causes.author as autor, causes.abstract as resumo,
             causes.local as local, causes.district as bairro, causes.created_at as quando, causes.views as visualizacoes, 
             causes.is_rejected as rejeitada, causes.protocol as protocolo
      FROM categories as cat JOIN causes on cat.id=causes.category_id
      WHERE #{where}
      ORDER BY quando desc       
    ")
    unless causes.empty?
      causes.reorder('causa_id','categoria','titulo','autor','resumo','local','bairro','quando','visualizacoes','rejeitada','protocolo')
      causes.save_as("public/reports/rejected_causes.csv")
    end
    causes
  end
  
  def self.get_users
    users = User.report_table_by_sql("
      SELECT username as user, name as nome, last_sign_in as ultimo_login, location as local, 
      IF(twitter_user_id is NULL,'não','sim') as twitter, IF(facebook_id is NULL,'não','sim') as facebook, 
      IF(google_email is NULL,'não','sim') as google
      FROM users
      ORDER BY ultimo_login desc       
    ")
    unless users.empty?
      users.reorder('user','nome','ultimo_login','local','twitter','facebook','google')
      users.save_as("public/reports/users.csv")
    end
    users
  end
  
  def self.get_total_twitter_users
    User.find_by_sql('SELECT count(*) as users FROM users WHERE twitter_user_id IS NOT NULL')
  end
  
  def self.get_total_facebook_users
    User.find_by_sql('SELECT count(*) as users FROM users WHERE facebook_id IS NOT NULL')    
  end
  
  def self.get_total_gmail_users
    User.find_by_sql('SELECT count(*) as users FROM users WHERE google_email IS NOT NULL')
  end
  
  def self.import_facebook_likes
    causes = Cause.find(:all,:conditions => "(is_rejected = 0) and (submited = 1) and ((last_likes_update < '#{48.hours.ago}') or (last_likes_update is null))", :limit => 50)
    threads = []
    causes.each do |cause|
      threads << spawn do
        url = "http://www.portoalegre.cc/causas/#{cause.category.name.urlize}/#{cause.title.urlize}/#{cause.id}"
        read_likes cause, url
        puts cause.id
      end
    end
    wait(threads)
    # SUGESTÃO PARA POOLING
    #    causes = Cause.find(:all,:conditions => {:is_rejected => 0}, :limit => 20)
    #    pool = ThreadPool.new(10) # up to 10 threads
    #    causes.each do |cause|
    #      url = "http://www.portoalegre.cc/causas/#{cause.category.name.urlize}/#{cause.title.urlize}/#{cause.id}"
    #      pool.process {read_likes cause, url}
    #      puts cause.id
    #    end    
    #    pool.join()
    # Retirado de http://snippets.dzone.com/posts/show/3276 (possivelmente tenha que se implementar a classe ThreadPool)

  end
  
  def self.read_likes cause, url
    response = ActiveSupport::JSON.decode(RestClient.get("https://graph.facebook.com/#{url}"))
    cause.update_attributes(:likes => response.include?("shares") ? response["shares"] : 0, :last_likes_update => Time.now)
  end
  
  def self.clean_causes
    causes = Cause.find(:all, :conditions => "submited = 0 AND updated_at < '#{30.minutes.ago}'")
    causes.each do |cause|
      RichContent.destroy_all("cause_id=#{cause.id}")
    end
    Cause.destroy_all("submited=0 and updated_at < '#{30.minutes.ago}'")
  end
  
end
