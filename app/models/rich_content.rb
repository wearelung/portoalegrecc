class RichContent < ActiveRecord::Base
  # campo kind diz o tipo do conteúdo
  # 1: foto
  # 2: video
  # 3: mapa
  belongs_to :cause
  validate :youtube_or_vimeo
  named_scope :videos, :conditions => { :kind => 2 }
  
  validates_attachment_content_type :photo, :content_type => %w( image/jpeg image/png image/gif image/pjpeg image/x-png )
  
  has_attached_file :photo,:url  => "/rich_content/:id/:style/:basename.:extension",
                  :path => "/public/rich_content/:id/:style/:basename.:extension",
                  :storage => :s3, :s3_credentials => "#{::Rails.root.to_s}/config/s3.yml"
                
  
  def youtube_or_vimeo
    if kind == 2
      unless /^.*((youtu.be\/)|(v\/)|(embed\/)|(watch\?))\??v?=?([^#\&\?]*).*/.match(url) ||
             /(http:\/\/)?(www\.)?vimeo.com\/(\d+)($|\/)/.match(url)
        errors.add(:video, "URL incorreta")
      end
    end
    true
  end

  def update_video_id_and_type
    if /^.*((youtu.be\/)|(v\/)|(embed\/)|(watch\?))\??v?=?([^#\&\?]*).*/.match url
      self.video_id = url.split("v=")[1][0..10]
      self.video_type = "youtube"
    else
      self.video_id = /(http:\/\/)?(www\.)?vimeo.com\/(\d+)($|\/)/.match(url)[3]
      self.video_type = "vimeo"
    end
  end
  
  def manage_image_deletion
    self.choosen = 1 - self.choosen
    self.save
  end
  
  def image_url
    self.url ? self.url : self.photo.url
  end
end
