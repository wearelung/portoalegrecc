class AddLastSignInToUsers < ActiveRecord::Migration
  def self.up    
    add_column :users, :last_sign_in, :datetime
  end

  def self.down
    remove_column :users, :last_sign_in
  end
end
