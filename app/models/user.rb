class User < ActiveRecord::Base
  after_create :send_email

  acts_as_reportable
  
  has_many :causes
  validates_presence_of :username
  validates_format_of :email, :with => /\A([^@\s]+)@((?:[-a-z0-9]+\.)+[a-z]{2,})\Z/i
  validates_uniqueness_of :username
  validates_uniqueness_of :twitter_user_id, :scope => [:google_email, :facebook_id]

  validate :only_one_social_network
  
  def only_one_social_network
    errors.add(:social_network, "Rede social deve ser escolhida") unless (twitter_user_id || google_email || facebook_id)
    errors.add(:social_network, "Somente uma rede social deve ser escolhida") unless ((twitter_user_id.blank? && google_email.blank?) || (twitter_user_id.blank? && facebook_id.blank?) || (google_email.blank? && facebook_id.blank?))
  end
  
  def send_email
    UsersMailer.deliver_registro(self) if !self.email.nil?
  end
  
  def self.authenticate(username, password)
    user = find_by_username_and_password(username, Digest::MD5.hexdigest(password))
    if user
      user.update_attribute(:last_sign_in, DateTime.now)
      user
    else
      nil
    end
  end
end
