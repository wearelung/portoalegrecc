module AdminHelper
  def cause_sortable(column, title=nil, direction=nil)
    css_class = column == cause_sort_column ? "current #{sort_direction}" : nil
    direction ||= column == cause_sort_column && sort_direction == "asc" ? "desc" : "asc"
    link_to title, {:sort => column, :direction => direction, :page => params[:page], 
      :abstract => params[:abstract], :author => params[:author], :category => params[:category],
      :title => params[:title] }, {:class => css_class}
  end
  
  def user_sortable(column, title=nil, direction=nil)
    css_class = column == user_sort_column ? "current #{sort_direction}" : nil
    direction ||= column == user_sort_column && sort_direction == "asc" ? "desc" : "asc"
    link_to title, {:sort => column, :direction => direction, :page => params[:page]}, {:class => css_class}
  end
end
