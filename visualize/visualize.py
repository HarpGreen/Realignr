import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import pandas as pd
import sys
import json

# 调试的时候记得把最新的这个文件复制到对应目录下
# 导入报错就换编译器，在win32

def visualize_multiple_point_sets(data_dict):
    """
    可视化多组点集
    
    参数:
    data_dict: 字典，包含不同点集的数据
               格式: {
                   'model_points': [[x,y,z], ...],
                   'actual_points': [[x,y,z], ...], 
                   'process_points_before': [[x,y,z], ...],
                   'process_points_after': [[x,y,z], ...]
               }
    """
    
    try:
        fig = plt.figure(figsize=(12, 8))
        ax = fig.add_subplot(111, projection='3d')

        def on_scroll(event):
            if event.inaxes != ax:
                return
            # Get current limits
            xlim = ax.get_xlim()
            ylim = ax.get_ylim()
            zlim = ax.get_zlim()

            # Calculate center
            xcenter = (xlim[0] + xlim[1]) / 2
            ycenter = (ylim[0] + ylim[1]) / 2
            zcenter = (zlim[0] + zlim[1]) / 2

            # Calculate current range
            xrange = xlim[1] - xlim[0]
            yrange = ylim[1] - ylim[0]
            zrange = zlim[1] - zlim[0]

            # Zoom factor
            zoom_factor = 0.9 if event.step > 0 else 1.1

            # New ranges
            new_xrange = xrange * zoom_factor
            new_yrange = yrange * zoom_factor
            new_zrange = zrange * zoom_factor

            # Set new limits
            ax.set_xlim(xcenter - new_xrange/2, xcenter + new_xrange/2)
            ax.set_ylim(ycenter - new_yrange/2, ycenter + new_yrange/2)
            ax.set_zlim(zcenter - new_zrange/2, zcenter + new_zrange/2)

            fig.canvas.draw_idle()

        fig.canvas.mpl_connect('scroll_event', on_scroll)

        colors = ['red', 'blue', 'green', 'orange']
        labels = ['Model points', 'Actual points', 'Process points (before)', 'Process points (after)']
        markers = ['o', 's', '^', 'D']
        keys = ['model_points', 'actual_points', 'process_points_before', 'process_points_after']
        
        all_points = []
        
        for i, key in enumerate(keys):
            if key in data_dict and data_dict[key]:
                points = np.array(data_dict[key])
                if points.size > 0:
                    # 绘制点
                    ax.scatter(points[:, 0], points[:, 1], points[:, 2], 
                              c=colors[i], s=50, alpha=0.8, marker=markers[i], label=labels[i])
                    
                    # 为每个点添加序号标签
                    for idx, point in enumerate(points):
                        offset = 0.02 * np.max(np.abs(points)) if points.size > 0 else 0.1
                        ax.text(point[0] + offset, point[1] + offset, point[2] + offset,
                               f'{labels[i][:2]}{idx+1}', fontsize=8, color=colors[i], alpha=0.7)
                    
                    all_points.extend(points)
        
        ax.set_xlabel('X Axis')
        ax.set_ylabel('Y Axis')
        ax.set_zlabel('Z Axis')
        ax.set_title('Coordinate Mapping Verification - Multiple Point Sets')
        
        # 自动调整视图
        if all_points:
            all_points = np.array(all_points)
            center_x = (np.min(all_points[:, 0]) + np.max(all_points[:, 0])) / 2
            center_y = (np.min(all_points[:, 1]) + np.max(all_points[:, 1])) / 2
            center_z = (np.min(all_points[:, 2]) + np.max(all_points[:, 2])) / 2
            
            max_range = max(
                np.max(all_points[:, 0]) - np.min(all_points[:, 0]),
                np.max(all_points[:, 1]) - np.min(all_points[:, 1]),
                np.max(all_points[:, 2]) - np.min(all_points[:, 2])
            ) / 2.0
            
            margin = max_range * 0.1
            max_range += margin
            
            ax.set_xlim(center_x - max_range, center_x + max_range)
            ax.set_ylim(center_y - max_range, center_y + max_range)
            ax.set_zlim(center_z - max_range, center_z + max_range)
        
        ax.set_box_aspect([1, 1, 1])
        ax.legend()
        
        plt.tight_layout()
        plt.show()
        
    except Exception as e:
        print(f"Visualization error: {e}")
        import traceback
        traceback.print_exc()

def visualize_points_and_vectors(filename):
    """
    Read CSV file and display points with vectors (original behavior preserved)
    
    Parameters:
    filename: CSV file name, each row contains 6 numbers: x,y,z,i,j,k
    """
    
    try:
        # 读取CSV文件
        data = pd.read_csv(filename, header=None, sep=',')
        
        if data.shape[1] < 6:
            print("Error: each row must contain at least 6 numbers (x,y,z,i,j,k)")
            return
            
        # 提取坐标和向量
        points = data.iloc[:, 0:3].values  # x,y,z
        vectors = data.iloc[:, 3:6].values  # i,j,k
        
        print(f"Read {len(points)} points")
        
        # 创建3D图形
        fig = plt.figure(figsize=(12, 8))
        ax = fig.add_subplot(111, projection='3d')
        
        # 绘制点（红圆点）
        ax.scatter(points[:, 0], points[:, 1], points[:, 2], 
                  c='red', s=50, alpha=0.8, marker='o')
        
        # 为每个点添加序号标签（行号，从1开始）
        for idx, point in enumerate(points):
            # 稍微偏移标签位置，避免与点重叠
            offset = 0.02 * np.max(np.abs(points))  # 基于数据范围的偏移
            ax.text(point[0] + offset, point[1] + offset, point[2] + offset,
                   str(idx + 1),  # 行号从1开始
                   fontsize=9, color='blue', alpha=0.8)
        
        # 绘制向量（从点到点+向量的红色线段）
        vector_scale = 1.0  # 向量缩放因子，可以根据需要调整
        
        for point, vector in zip(points, vectors):
            # 计算向量终点
            end_point = point + vector * vector_scale
            
            # 绘制线段
            ax.plot([point[0], end_point[0]], 
                   [point[1], end_point[1]], 
                   [point[2], end_point[2]], 
                   c='red', linewidth=2, alpha=0.7)
        
        # 设置图形属性
        ax.set_xlabel('X Axis')
        ax.set_ylabel('Y Axis')
        ax.set_zlabel('Z Axis')
        ax.set_title('3D Points and Vectors Display (with Index)')
        
        # 自动居中显示所有点和向量
        # 计算包含所有点和向量终点的边界
        vector_ends = points + vectors * vector_scale
        all_points = np.vstack([points, vector_ends])
        
        # 计算中心点
        center_x = (np.min(all_points[:, 0]) + np.max(all_points[:, 0])) / 2
        center_y = (np.min(all_points[:, 1]) + np.max(all_points[:, 1])) / 2
        center_z = (np.min(all_points[:, 2]) + np.max(all_points[:, 2])) / 2
        
        # 计算最大范围
        max_range = max(
            np.max(all_points[:, 0]) - np.min(all_points[:, 0]),
            np.max(all_points[:, 1]) - np.min(all_points[:, 1]),
            np.max(all_points[:, 2]) - np.min(all_points[:, 2])
        ) / 2.0
        
        # 添加一些边距（10%的额外空间）
        margin = max_range * 0.1
        max_range += margin
        
        # 设置坐标轴范围以居中显示
        ax.set_xlim(center_x - max_range, center_x + max_range)
        ax.set_ylim(center_y - max_range, center_y + max_range)
        ax.set_zlim(center_z - max_range, center_z + max_range)
        
        # 设置相等的纵横比
        ax.set_box_aspect([1, 1, 1])
        
        # 显示图形
        plt.tight_layout()
        plt.show()
        
        return points, vectors
        
    except FileNotFoundError:
        print(f"Error: file not found '{filename}'")
    except Exception as e:
        print(f"An error occurred: {e}")
        import traceback
        traceback.print_exc()

def main():
    """
    Main function, supports two modes:
    1. Command-line argument specifies a filename (original behavior)
    2. Read JSON data from stdin (new behavior)
    """
    if len(sys.argv) > 1:
        filename = sys.argv[1]
        visualize_points_and_vectors(filename)
    else:
        # New mode: read JSON data from stdin for multi-point-set visualization
        try:
            input_data = sys.stdin.read()
            data_dict = json.loads(input_data)
            visualize_multiple_point_sets(data_dict)
        except json.JSONDecodeError:
            print("Error: unable to parse JSON input data")
        except Exception as e:
            print(f"Visualization error: {e}")

if __name__ == "__main__":
    main()
