class AddLikesandLikesUpdateToCauses < ActiveRecord::Migration
  def self.up
    add_column :causes, :likes, :integer, :default => 0
    add_column :causes, :last_likes_update, :datetime    
  end

  def self.down
    remove_column :causes, :likes
    remove_column :causes, :last_likes_update
  end
end
