import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import pandas as pd
import sys

def visualize_points_and_vectors(filename):
    """
    读取CSV文件并显示点及其向量
    
    参数:
    filename: CSV文件名，每行包含6个数字: x,y,z,i,j,k
    """
    
    try:
        # 读取CSV文件
        data = pd.read_csv(filename, header=None, sep=',')
        
        # 确保有足够的列
        if data.shape[1] < 6:
            print("错误: 每行至少需要6个数字 (x,y,z,i,j,k)")
            return
            
        # 提取坐标和向量
        points = data.iloc[:, 0:3].values  # x,y,z
        vectors = data.iloc[:, 3:6].values  # i,j,k
        
        print(f"读取了 {len(points)} 个点")
        
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
        ax.set_xlabel('X轴')
        ax.set_ylabel('Y轴')
        ax.set_zlabel('Z轴')
        ax.set_title('点和向量三维显示（带序号）')
        
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
        print(f"错误: 找不到文件 '{filename}'")
    except Exception as e:
        print(f"发生错误: {e}")
        import traceback
        traceback.print_exc()

def main():
    """
    主函数，可以从命令行接收文件名
    """
    if len(sys.argv) > 1:
        filename = sys.argv[1]
    else:
        print("未指定文件名")
        print("使用方法: python script.py <filename>")
        return
    visualize_points_and_vectors(filename)

if __name__ == "__main__":
    main()
